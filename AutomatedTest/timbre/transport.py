"""
Layer 0: a line-oriented pipe to the target.

Two implementations behind one interface. SerialTransport talks to real
hardware; HostTransport runs the host build of the CLI as a subprocess.
Everything above this layer is identical either way, which is what lets the
bulk of the suite run at host speed and only the hardware-dependent tests
wait on a board.

A background reader thread owns the read side, so callers can wait for a
specific line with a timeout without ever blocking the interpreter.
"""

from __future__ import annotations

import queue
import subprocess
import threading
import time
from abc import ABC, abstractmethod


class TransportError(RuntimeError):
    pass


# Prefix for the synthetic line Transport._read_loop() manufactures when the
# target sends an unreasonably long run of bytes with no newline at all - a
# real, observed board fault mode ("just continuously outputs spaces") that
# otherwise looks exactly like silence to every caller above this layer,
# since read_line()/drain() only ever yield newline-terminated lines.
UNTERMINATED_STREAM_MARKER = "\x00UNTERMINATED_STREAM\x00"

# No legitimate single response line on this board approaches this length.
# "help" with no argument is dozens of short lines, not one long one - but
# "words" *is* one long line, every command name on the board space-
# separated with no newline until the very end, and it's the longest
# single line this board ever sends. Measured directly off the wire at
# 1054 bytes (confirmed live: an earlier, lower threshold here caused
# false positives that took down word_names() itself, since 512 turned
# out to be smaller than this real, legitimate line). It will keep
# growing as words are added to the firmware, so this leaves several
# times that much headroom rather than trimming close to today's actual
# number - comfortably above any real line, still small enough to catch
# a genuine unterminated flood fast rather than waiting it out.
MAX_UNTERMINATED_BUFFER = 4096


def wait_until_responsive(transport: "Transport", probe_cmd: str, timeout: float) -> None:
    """Poll with a real command until the target actually answers.

    Real hardware has shown an extended unresponsive window right after
    the port opens - tens of seconds, observed up to roughly a minute -
    before the CLI starts answering at all. Diving straight into real
    traffic during that window means every attempt burns a full read
    timeout before failing, which looks exactly like a link/corruption
    problem when the board is simply still not listening yet. Shared by
    the pytest transport fixture and any standalone script (e.g.
    tools/checkpoint_run.py) that opens its own connection.

    A known board fault mode looks identical to silence from here unless
    specifically checked for: a hang that continuously emits spaces with
    no newline, ever. Transport._read_loop() already turns a very long
    unterminated run into a synthetic UNTERMINATED_STREAM line rather than
    burying it forever in an internal buffer (see MAX_UNTERMINATED_BUFFER)
    - this is what actually looks for that line and gives it a specific
    diagnosis instead of the generic timeout below, which used to be the
    only outcome and read exactly like "board never came up."
    """
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        transport.write_line(probe_cmd)
        lines = transport.drain(quiet_for=0.3, limit=1.0)
        if any(line.startswith(UNTERMINATED_STREAM_MARKER) for line in lines):
            raise TransportError(
                f"target is emitting a continuous unterminated stream (no "
                f"newline) instead of answering {probe_cmd!r} - the known "
                f"'spaces forever' fault mode, not silence. This needs a "
                f"physical reset; resending commands will not recover it."
            )
        if lines:
            return
    raise TransportError(
        f"target never answered {probe_cmd!r} within timeout={timeout}s. "
        f"Board may still be booting, wedged, or something else is holding the port."
    )


def warn_if_tty_node(port: str) -> None:
    """The macOS tty/cu trap, caught before it costs anyone an afternoon.

    Both nodes exist for every serial adapter. Opening tty.* blocks waiting
    for carrier detect, which most USB-serial adapters never assert, so the
    process hangs silently with no error and no output. cu.* is the callout
    node and opens immediately.
    """
    import sys

    if sys.platform == "darwin" and port.startswith("/dev/tty."):
        suggestion = port.replace("/dev/tty.", "/dev/cu.", 1)
        raise TransportError(
            f"{port} is a tty node. On macOS this blocks waiting for "
            f"carrier detect and will hang rather than fail.\n"
            f"Use {suggestion} instead."
        )


def check_port_not_already_open(port: str) -> None:
    """Refuse to share the port with another process.

    A second process (a terminal, miniterm, a leftover harness run) holding
    the port at the same time doesn't reliably fail loudly - both sides can
    read each other's echoes and interleave writes, which looks exactly
    like a wedged or crashed target: silence, or garbled responses, with no
    error anywhere pointing at the real cause. Checked with lsof before
    opening, best-effort: if lsof isn't available (non-macOS/Linux, or not
    on PATH), this is a no-op and exclusive=True on the actual open is the
    only guard left.
    """
    import subprocess

    try:
        out = subprocess.run(
            ["lsof", "-t", port],
            capture_output=True, text=True, timeout=2,
        ).stdout.strip()
    except (FileNotFoundError, subprocess.TimeoutExpired, OSError):
        return
    if not out:
        return

    holders = []
    for pid in out.splitlines():
        try:
            name = subprocess.run(
                ["ps", "-p", pid, "-o", "comm="],
                capture_output=True, text=True, timeout=2,
            ).stdout.strip()
        except (subprocess.TimeoutExpired, OSError):
            name = ""
        holders.append(f"pid {pid} ({name or 'unknown process'})")

    raise TransportError(
        f"{port} is already open by: {', '.join(holders)}\n"
        "Close that first - a terminal, miniterm, or a leftover test run. "
        "A shared port doesn't fail loudly; it just looks like a wedged or "
        "crashed target."
    )


class Transport(ABC):
    def __init__(
        self,
        encoding: str = "ascii",
        tx_eol: str = "\r\n",
        conversation_log: str | None = None,
    ):
        self.encoding = encoding
        self.tx_eol = tx_eol
        self._lines: queue.Queue[str] = queue.Queue()
        self._buf = ""
        self._reader: threading.Thread | None = None
        self._stop = threading.Event()
        self._open_flag = False
        # Every line sent and every line received, tagged and in true
        # chronological order - the actual conversation a test run has with
        # the target, readable afterward as a transcript rather than
        # reconstructed from pytest's own pass/fail output. Optional: only
        # opened when a path is given (see --session-log in conftest.py).
        self._conversation_log_path = conversation_log
        self._conversation_log = None

    # -- subclass contract ------------------------------------------------
    @abstractmethod
    def _open(self) -> None: ...

    @abstractmethod
    def _close(self) -> None: ...

    @abstractmethod
    def _write_bytes(self, data: bytes) -> None: ...

    @abstractmethod
    def _read_bytes(self) -> bytes:
        """Block until some bytes arrive, or return b'' on timeout/EOF."""

    # -- public API -------------------------------------------------------
    def open(self) -> None:
        if self._open_flag:
            return
        if self._conversation_log_path:
            self._conversation_log = open(self._conversation_log_path, "a", buffering=1)
            self._log_start = time.monotonic()
            self._conversation_log.write(
                f"\n=== session started {time.strftime('%Y-%m-%d %H:%M:%S')} "
                f"({self!r}) ===\n"
            )
        self._open()
        self._open_flag = True
        self._stop.clear()
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()

    def close(self) -> None:
        if not self._open_flag:
            return
        self._stop.set()
        self._close()
        if self._reader:
            self._reader.join(timeout=1.0)
        self._open_flag = False
        if self._conversation_log:
            self._conversation_log.write(
                f"=== session ended {time.strftime('%Y-%m-%d %H:%M:%S')} ===\n"
            )
            self._conversation_log.close()
            self._conversation_log = None

    def write_line(self, line: str) -> None:
        if not self._open_flag:
            raise TransportError("transport not open")
        self._log_conversation("TX", line)
        self._write_bytes((line + self.tx_eol).encode(self.encoding, "replace"))

    def write_raw(self, text: str) -> None:
        """Send exactly these characters, no line ending appended.

        Used by link-layer tests that deliberately send malformed input.
        """
        if not self._open_flag:
            raise TransportError("transport not open")
        # A trailing CR/LF (e.g. resetcli's own EOL, sent unframed) is the
        # common case and isn't interesting on its own - RX lines are
        # already shown without theirs. Config's tx_eol and this
        # transport's own tx_eol can differ (HostTransport uses "\n" on the
        # wire while callers still build text with cfg's "\r\n"), so this
        # strips CR/LF generically rather than matching one exact string.
        # Only trailing whitespace is touched, so genuine embedded control
        # bytes (deliberate garbage/fuzz tests) still fall through to
        # _log_conversation's repr().
        shown = text.rstrip("\r\n")
        self._log_conversation("TX", shown)
        self._write_bytes(text.encode(self.encoding, "replace"))

    def _log_conversation(self, direction: str, text: str) -> None:
        """Plain text when it's readable, repr() only when it genuinely
        isn't - deliberate binary-noise tests are the only source of
        unprintable content here, and normal command/response text should
        read like what it is: a conversation, not a Python repr of one.
        """
        if not self._conversation_log:
            return
        elapsed = time.monotonic() - self._log_start
        shown = text if text.isprintable() else repr(text)
        self._conversation_log.write(f"[{elapsed:8.3f}] {direction} {shown}\n")
        self._conversation_log.flush()

    def log_marker(self, text: str) -> None:
        """Write a plain divider into the conversation log, e.g. a test
        boundary. Not a TX or RX event, so it isn't timestamped or tagged
        like one - just a readable seam in the transcript.
        """
        if self._conversation_log:
            self._conversation_log.write(f"\n----- {text} -----\n")
            self._conversation_log.flush()

    def read_line(self, timeout: float) -> str | None:
        try:
            return self._lines.get(timeout=timeout)
        except queue.Empty:
            return None

    def drain(self, quiet_for: float = 0.15, limit: float = 2.0) -> list[str]:
        """Consume pending lines until the target goes quiet."""
        collected: list[str] = []
        deadline = time.monotonic() + limit
        while time.monotonic() < deadline:
            line = self.read_line(timeout=quiet_for)
            if line is None:
                break
            collected.append(line)
        return collected

    # -- internals --------------------------------------------------------
    def _read_loop(self) -> None:
        while not self._stop.is_set():
            try:
                chunk = self._read_bytes()
            except Exception:
                break
            if not chunk:
                continue
            self._buf += chunk.decode(self.encoding, "replace")
            while "\n" in self._buf:
                line, self._buf = self._buf.split("\n", 1)
                line = line.rstrip("\r")
                self._log_conversation("RX", line)
                self._lines.put(line)
            if len(self._buf) > MAX_UNTERMINATED_BUFFER:
                # This much data with no newline in sight isn't a slow
                # response, it's the "continuously outputs spaces" fault
                # mode - surface it as a distinguishable synthetic line
                # instead of silently growing self._buf forever, which
                # otherwise makes this indistinguishable from the board
                # simply not answering at all. See wait_until_responsive().
                stuck, self._buf = self._buf, ""
                preview = stuck[:60]
                self._log_conversation(
                    "RX", f"<{len(stuck)} bytes, no newline, {stuck.count(' ')} spaces> {preview!r}..."
                )
                self._lines.put(f"{UNTERMINATED_STREAM_MARKER}{len(stuck)} bytes: {preview!r}...")

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, *exc):
        self.close()


class SerialTransport(Transport):
    def __init__(
        self,
        port: str,
        baud: int,
        tx_eol: str = "\r\n",
        inter_write_delay: float = 0.0,
        conversation_log: str | None = None,
    ):
        super().__init__(tx_eol=tx_eol, conversation_log=conversation_log)
        self.port = port
        self.baud = baud
        # Paces consecutive writes (e.g. the open/close marker lines a
        # single execute() sends back to back). Observed on real hardware:
        # under sustained back-to-back traffic with no flow control, bytes
        # get dropped or corrupted (a response arriving as "ype<- eh?"
        # instead of "type<- eh?", frame_lost on an otherwise-fine command).
        # A few milliseconds gives the board's echo/RX handling time to
        # drain before the next line lands. Not used by HostTransport -
        # a subprocess pipe has no such ceiling.
        self.inter_write_delay = inter_write_delay
        self._ser = None

    def _open(self) -> None:
        # Checked before the pyserial import, so the diagnostic still fires
        # on a machine where the dependency is missing.
        self._warn_if_tty_node()
        self._check_not_already_open()

        import serial  # pyserial

        try:
            # exclusive=True (POSIX only, ignored elsewhere) is a second
            # layer on top of _check_not_already_open: if something opens
            # the port in the gap between that check and this call, the OS
            # itself refuses the second open instead of silently sharing it.
            self._ser = serial.Serial(self.port, self.baud, timeout=0.05, exclusive=True)
        except Exception as exc:
            raise TransportError(
                f"could not open {self.port}: {exc}\n"
                f"Available ports: {self._available() or 'none found'}"
            ) from exc
        self._ser.reset_input_buffer()
        self._ser.reset_output_buffer()

    @staticmethod
    def _available() -> str:
        try:
            from serial.tools import list_ports

            return ", ".join(p.device for p in list_ports.comports()) or ""
        except Exception:
            return ""

    def _check_not_already_open(self) -> None:
        check_port_not_already_open(self.port)

    def _warn_if_tty_node(self) -> None:
        warn_if_tty_node(self.port)

    def _close(self) -> None:
        if self._ser:
            self._ser.close()
            self._ser = None

    def _write_bytes(self, data: bytes) -> None:
        self._ser.write(data)
        self._ser.flush()
        if self.inter_write_delay:
            time.sleep(self.inter_write_delay)

    def _read_bytes(self) -> bytes:
        n = self._ser.in_waiting or 1
        return self._ser.read(n)

    def __repr__(self) -> str:
        return f"serial {self.port} @ {self.baud}"


class HostTransport(Transport):
    """Runs the host build of the CLI over stdin/stdout.

    Same protocol, no board, no 115200 baud ceiling. Use this for the
    parser, stack, and command-matrix tests; keep hardware for anything
    that touches real peripherals or real time.
    """

    def __init__(self, argv: list[str], tx_eol: str = "\n", conversation_log: str | None = None):
        super().__init__(tx_eol=tx_eol, conversation_log=conversation_log)
        self.argv = argv
        self._proc: subprocess.Popen | None = None

    def _open(self) -> None:
        self._proc = subprocess.Popen(
            self.argv,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            bufsize=0,
        )

    def _close(self) -> None:
        if self._proc:
            try:
                self._proc.terminate()
                self._proc.wait(timeout=2.0)
            except Exception:
                self._proc.kill()
            self._proc = None

    def _write_bytes(self, data: bytes) -> None:
        self._proc.stdin.write(data)
        self._proc.stdin.flush()

    def _read_bytes(self) -> bytes:
        # bufsize=0 makes stdout a raw stream, so read(n) is a single
        # syscall returning whatever is available rather than blocking for
        # a full buffer. Portable across platforms, unlike os.read on a
        # Windows pipe descriptor.
        #
        # Reading a byte at a time here made the host suite roughly thirty
        # times slower than it needed to be.
        try:
            return self._proc.stdout.read(4096) or b""
        except (OSError, ValueError):
            return b""

    def __repr__(self) -> str:
        return f"host {' '.join(self.argv)}"

"""
Layer 1: request/response correlation.

The target frames its own responses. Because the CLI executes multiple words
per line left to right, we send:

    <mark> TH-B0007 <command under test> <mark> TH-E0007

The interpreter runs the opening mark, then the command, then the closing
mark. Everything the target emits between those two marker lines is that
command's output. This removes every timing assumption from the harness:
there is no "sleep and hope", no readline race, and an event that fires
mid-command is recognised by its async prefix rather than mistaken for a
result.

It also means a hung command is detected as a missing end marker rather than
as silence, which is a much more specific failure.
"""

from __future__ import annotations

import itertools
import re
import time
from dataclasses import dataclass, field

from .codec import Codec
from .config import TargetConfig
from .transport import Transport


class ProtocolError(RuntimeError):
    pass


@dataclass
class Response:
    command: str
    seq: int
    sent: str
    lines: list[str] = field(default_factory=list)
    events: list[str] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)
    elapsed: float = 0.0
    crc_failures: int = 0
    timed_out: bool = False
    frame_lost: bool = False  # command line discarded before it ran

    @property
    def ok(self) -> bool:
        return (
            not self.errors
            and not self.timed_out
            and not self.frame_lost
            and self.crc_failures == 0
        )

    @property
    def text(self) -> str:
        return "\n".join(self.lines)

    def one_line(self) -> str:
        if len(self.lines) != 1:
            raise ProtocolError(
                f"expected exactly one output line from {self.command!r}, "
                f"got {len(self.lines)}: {self.lines!r}"
            )
        return self.lines[0]

    def as_int(self) -> int:
        return int(self.one_line().strip(), 0)

    def __str__(self) -> str:
        state = "ok" if self.ok else "FAIL"
        return f"<Response {self.command!r} {state} lines={len(self.lines)}>"


class Session:
    def __init__(self, transport: Transport, codec: Codec, cfg: TargetConfig):
        self.transport = transport
        self.codec = codec
        self.cfg = cfg
        self._seq = itertools.count(1)
        self._error_re = re.compile(cfg.error_pattern)
        self.orphan_events: list[str] = []

    # ------------------------------------------------------------------
    def execute(self, command: str, timeout: float | None = None) -> Response:
        """Send one command line, framed, and collect its response.

        Retries exactly once on frame_lost: the begin marker never arrived,
        meaning the open line was dropped or corrupted in transit rather
        than rejected by the interpreter. Observed under sustained
        back-to-back traffic on real hardware. A second frame_lost in a row
        is a real problem, not transient noise, and is returned as-is.
        """
        resp = self._execute_once(command, timeout)
        if resp.frame_lost:
            self.resync()
            resp = self._execute_once(command, timeout)
        return resp

    def _execute_once(self, command: str, timeout: float | None) -> Response:
        seq = next(self._seq)
        begin = f"{self.cfg.begin_marker}{seq:04d}"
        end = f"{self.cfg.end_marker}{seq:04d}"

        # The end marker goes on its OWN line, not appended to the command
        # line. An error aborts the remainder of the line it occurred on, so
        # a trailing marker would never execute and every negative test would
        # report a timeout instead of the error the target correctly sent.
        # A separate line always runs.
        #
        # The payoff: `timed_out` now means the interpreter is genuinely
        # stuck, rather than meaning "the command failed", which are very
        # different diagnoses.
        open_line = self.codec.encode(f"{self._mark_line(begin)} {command}")
        close_line = self.codec.encode(self._mark_line(end))

        resp = Response(command=command, seq=seq, sent=open_line)
        limit = self.cfg.max_line_length
        if limit and len(open_line) > limit:
            raise ProtocolError(
                f"framed command is {len(open_line)} chars, over the target's "
                f"{limit}-char input buffer. Framing overhead is "
                f"{len(open_line) - len(command)} chars. Send this command "
                f"unframed via Session.raw(), or shorten it."
            )
        started = time.monotonic()
        self.transport.write_line(open_line)
        self.transport.write_line(close_line)
        self._collect(resp, begin, end, timeout or self._timeout_for(command))
        resp.elapsed = time.monotonic() - started
        return resp

    def execute_block(self, lines: list[str], timeout: float | None = None) -> Response:
        """Run several lines under a single frame.

        The begin marker rides on the first line and the end marker follows
        the last, so everything in between is one correlated response. Use
        it for macro definitions that span lines, and for setup sequences
        that should be read as one step rather than a stack of unrelated
        commands.

        Timeout covers the whole block, not each line. Retries once on
        frame_lost - see execute().
        """
        resp = self._execute_block_once(lines, timeout)
        if resp.frame_lost:
            self.resync()
            resp = self._execute_block_once(lines, timeout)
        return resp

    def _execute_block_once(self, lines: list[str], timeout: float | None) -> Response:
        seq = next(self._seq)
        begin = f"{self.cfg.begin_marker}{seq:04d}"
        end = f"{self.cfg.end_marker}{seq:04d}"

        body = [l for l in (line.strip() for line in lines) if l]
        if not body:
            raise ProtocolError("execute_block called with no content")

        wire = [self.codec.encode(f"{self._mark_line(begin)} {body[0]}")]
        wire += [self.codec.encode(l) for l in body[1:]]
        wire.append(self.codec.encode(self._mark_line(end)))

        limit = self.cfg.max_line_length
        for line in wire:
            if limit and len(line) > limit:
                raise ProtocolError(
                    f"block line is {len(line)} chars, over the target's "
                    f"{limit}-char input buffer: {line[:60]!r}..."
                )

        resp = Response(command=" ; ".join(body), seq=seq, sent="\n".join(wire))
        started = time.monotonic()
        for line in wire:
            self.transport.write_line(line)
        self._collect(resp, begin, end, timeout or self.cfg.command_timeout * len(wire))
        resp.elapsed = time.monotonic() - started
        return resp

    def raw(self, text: str, quiet_for: float = 0.3) -> list[str]:
        """Send characters verbatim and return whatever comes back.

        No framing, no CRC. For link-layer and fuzz tests that deliberately
        send input the framing itself could not survive.
        """
        self.transport.write_raw(text)
        return self.transport.drain(quiet_for=quiet_for)

    def resync(self) -> None:
        """Return the interpreter to a known state after a failure."""
        self.transport.write_raw(self.cfg.tx_eol)
        self.transport.drain(quiet_for=0.2)

    # ------------------------------------------------------------------
    def _mark_line(self, marker: str) -> str:
        return self.cfg.mark_template.format(marker=marker)

    def _timeout_for(self, command: str) -> float:
        first = command.strip().split()[0] if command.strip() else ""
        return self.cfg.command_timeout_overrides.get(first, self.cfg.command_timeout)

    def _collect(self, resp: Response, begin: str, end: str, timeout: float) -> None:
        deadline = time.monotonic() + timeout
        in_body = False

        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                resp.timed_out = True
                return

            line = self.transport.read_line(timeout=remaining)
            if line is None:
                resp.timed_out = True
                return

            decoded = self.codec.decode(line)
            if decoded.crc_present and not decoded.crc_ok:
                resp.crc_failures += 1
                # Keep the corrupted text for diagnosis, do not interpret it.
                resp.lines.append(f"<CRC FAIL> {decoded.payload}")
                continue

            text = self._strip_prompt(decoded.payload)
            stripped = text.strip()

            # Markers must arrive alone on a line (requirement R1). This is
            # what makes framing survive command echo: the echoed command
            # line contains the marker but is not equal to it.
            if not in_body:
                if stripped == begin:
                    in_body = True
                elif stripped == end:
                    # The closing marker arrived but the opening one
                    # never did, so the command line was discarded
                    # before it executed - buffer overrun, CRC reject,
                    # or a framing error. A specific diagnosis, returned
                    # instantly instead of burning the full timeout.
                    resp.frame_lost = True
                    return
                continue

            if stripped == end:
                return

            if self._is_event(stripped):
                resp.events.append(stripped)
                continue

            if self._error_re.search(text):
                resp.errors.append(stripped)

            resp.lines.append(text)

    def _strip_prompt(self, line: str) -> str:
        prompt = self.cfg.prompt
        if prompt and line.startswith(prompt):
            return line[len(prompt):]
        return line

    def _is_event(self, line: str) -> bool:
        prefix = self.cfg.async_prefix
        return bool(prefix) and line.startswith(prefix)

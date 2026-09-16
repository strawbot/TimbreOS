"""
Every target-specific value lives here. Nothing else in the harness should
contain a literal command name, prompt string, or timeout.

Work top to bottom and fill in the TODOs against the real TimbreOS CLI.
"""

from __future__ import annotations

import pathlib
import sys
from dataclasses import dataclass, field

# Anchored to this file so the harness works from any working directory.
# PyCharm run configurations default to the project root, which is usually
# one level above the harness, and a relative path would silently fail there.
_HARNESS_ROOT = pathlib.Path(__file__).resolve().parent.parent
_FAKE_TARGET = _HARNESS_ROOT / "tools" / "fake_target.py"


@dataclass
class TargetConfig:
    # ------------------------------------------------------------------
    # Link
    # ------------------------------------------------------------------
    # macOS: use the /dev/cu.* node, NOT /dev/tty.*.
    #
    # Both appear for the same adapter. Opening the tty. node blocks waiting
    # for carrier detect, which most USB-serial adapters never assert, so the
    # run hangs with no error and no output. The cu. node opens immediately.
    #
    # Find yours with:  ls /dev/cu.*
    # or:               python -m serial.tools.list_ports -v
    #
    # This is an FTDI adapter, so the suffix is the chip's EEPROM serial
    # number rather than a USB port position. It stays the same across
    # reboots and whichever port you plug into, which is why it can be
    # hard-coded here. (cu.usbmodem* names are position-derived and do
    # move around; those are worth passing with --port instead.)
    port: str = "/dev/cu.usbserial-FTVGKHLB3"
    baud: int = 115200
    # Line ending the target expects on input.
    tx_eol: str = "\r\n"
    # True if the target echoes characters back as they are received.
    echo_enabled: bool = True

    # Gap held between consecutive writes to the serial port (e.g. the
    # open/close marker lines a single framed command sends back to back).
    # Real hardware, under sustained back-to-back traffic with no flow
    # control, has dropped or corrupted bytes without it: a response
    # arriving as "ype<- eh?" instead of "type<- eh?", or a frame_lost on
    # an otherwise-fine command. Only used by SerialTransport.
    command_pacing: float = 0.01

    # Host build of the CLI, used with --transport=host. Should speak the
    # identical protocol on stdin/stdout.
    # sys.executable, not "python3": on Windows there is usually no
    # `python3` on PATH, and inside a virtualenv it may resolve to the
    # wrong interpreter.
    host_binary: list[str] = field(
        default_factory=lambda: [sys.executable, str(_FAKE_TARGET)]
    )

    # ------------------------------------------------------------------
    # Framing / correlation
    # ------------------------------------------------------------------
    # How to print a literal marker line to the console.
    #
    # REQUIREMENT R1: printing the marker must emit it alone on its own line
    # (leading newline if needed). The harness identifies markers by testing
    # `line.strip() == token`, which is what makes framing immune to echo
    # and to interleaved async output.
    #
    # This board's `echo` word is a filesystem command - <name> <text...>
    # appends text to a file (see board-NanoR4/Board/wordlist.c) - not a
    # console-print word, so it cannot frame commands. The CLI's quoted
    # string literal can: `"text"` pushes the string's address, and
    # `dup strlen type` prints it from address+count. Net stack effect is
    # zero (quote pushes one value, strlen replaces it with a count, type
    # consumes both), so it is safe to splice around a test's own stack use.
    mark_template: str = '"{marker}" dup strlen type cr'

    begin_marker: str = "TH-B"
    end_marker: str = "TH-E"

    # TODO: prefix the firmware puts on unsolicited event/log lines.
    #
    # REQUIREMENT R2: async output must be distinguishable from command
    # results. Without this, an event that fires mid-command is
    # indistinguishable from that command's output. Set to None only if the
    # firmware genuinely never emits unsolicited text.
    async_prefix: str | None = "!"

    # Interactive prompt, stripped from output if present.
    prompt: str = "ar: "

    # The target's input line buffer size, in characters: LINE_LENGTH in
    # board-NanoR4/Board/project_defs.h. Was left at a guessed 512, which is
    # bigger than the real 400-byte tib buffer - the pre-flight check below
    # never tripped for lines that actually overran the target, so those
    # tests just hung waiting for a response the board had already
    # discarded.
    #
    # Framing is not free: each command is sent as
    #   "<mark> <begin-token> <command>"  plus the CRC suffix
    # which is roughly 20-30 characters of overhead. If the framed line
    # overruns the target's buffer the whole line is discarded, marker
    # included, and the command never runs. The harness checks this before
    # transmitting so the failure is a clear message rather than a timeout.
    max_line_length: int = 400

    # ------------------------------------------------------------------
    # Error detection
    # ------------------------------------------------------------------
    # TODO: regex matching any line the CLI emits to report a failure.
    # Must match ALL error paths: unknown word, stack underflow, bad
    # argument, range violation.
    #
    # This firmware's errors don't share a leading sigil at all, so the
    # pattern cannot be anchored to the start of the line - checked every
    # msg() call in cli.c:
    #   unknown word:            "<word><- eh?"        (quit() aborts the
    #                             rest of the line, stacks cleared)
    #   stack over/underflow:    " stack overflow "/" stack underflow "
    #   return-stack over/under: " return stack overflow "/"...underflow "
    #                             (superstring of the two above)
    #   divide/modulo by zero:   " div/0 "/" mod/0 "
    # Only the unknown-word path sets the interpreter's error flag and
    # aborts the line; push()/pop() print their message and keep going.
    # Whatever you change this to, test_error_detection_matches_real_errors
    # below keeps it honest.
    error_pattern: str = r"eh\?|stack overflow|stack underflow|div/0|mod/0"

    # ------------------------------------------------------------------
    # Introspection
    # ------------------------------------------------------------------
    # TODO: word that lists every command name, whitespace separated.
    words_cmd: str = "words"
    # TODO: word that prints name / args / results / help for one command.
    # The harness sends: "<describe_cmd> <name>"
    describe_cmd: str = "help"

    # ------------------------------------------------------------------
    # Parameter stack
    # ------------------------------------------------------------------
    # Prints depth AND every value. The values are what make sentinel
    # testing possible: with only a depth you can tell that a command
    # touched the stack, but not which values it took.
    stack_cmd: str = ".s"
    # Word that empties the parameter stack. This board has no "clear" -
    # the real word is "sp!" (see board-NanoR4/Board/wordlist.c / cli.c
    # spStore()).
    clear_stack_cmd: str = "sp!"

    # Full interpreter reset: removes macros, resets stacks, cancels naps,
    # stops interpreting, resets the input parser. This is the isolation
    # primitive the fixtures use between tests - it covers state that
    # clearing the parameter stack does not, notably pending naps and
    # leftover macros, both of which otherwise make tests depend on
    # execution order.
    #
    # Because it stops interpreting, anything after it on the same line is
    # discarded. The harness sends it alone, unframed, and resyncs.
    reset_cmd: str = "resetcli"
    # Quiet period that marks the end of reset output. The reset is issued
    # twice per test by the isolation fixture, so this value is multiplied
    # across the whole suite - at 0.2s it added two minutes. Raise it only
    # if hardware genuinely needs longer to settle.
    reset_settle: float = 0.05
    # Depth the stack must return to between tests.
    baseline_depth: int = 0

    # Matches "4 stack items: 1 2 3 2" - depth is the first integer and the
    # words in between are ignored.
    #   "count_first" -> "4 stack items: 1 2 3 2"
    #   "bracket"     -> "<4> 1 2 3 2"
    #   "values_only" -> "1 2 3 2"
    stack_format: str = "count_first"

    # Which end of the printed list is the top of the stack.
    #
    #   "bottom_first" -> "1 2 3 2", rightmost value is the top (Forth `.s`)
    #   "top_first"    -> leftmost value is the top
    #
    # Getting this wrong does not produce an error. It silently inverts
    # every stack assertion, so `swap` looks broken while `dup` still
    # passes, and the conformance sweep reports confident nonsense.
    #
    # Do not set it from the stack comments. test_stack_display_order
    # settles it empirically against the target: push three distinct
    # values, drop one, and see which end lost a value.
    #
    # Internally the harness always normalises to bottom-first, so nothing
    # above the parser needs to care.
    stack_display_order: str = "bottom_first"

    # Sentinel values pushed beneath real parameters. Improbable enough
    # that seeing one in a result means a command reached below its own
    # arguments. 0x5EED0000 | index.
    sentinel_base: int = 0x5EED0000

    # ------------------------------------------------------------------
    # Macros
    # ------------------------------------------------------------------
    # TODO: the macro definition syntax.
    #
    # Worth wiring up: at 115200 a long test line costs real time, and the
    # sweep tests send thousands of them. Defining a sequence once and
    # calling it by name collapses that traffic, and makes multi-step
    # fixtures readable instead of a wall of repeated words.
    macro_begin_cmd: str = ":"
    macro_end_cmd: str = ";"
    # TODO: word that removes a macro, or None if the CLI cannot.
    # Without it, macros accumulate across a session and tests stop being
    # independent.
    macro_forget_cmd: str | None = "forget"

    # ------------------------------------------------------------------
    # Execution-time instrument
    # ------------------------------------------------------------------
    # Word that prints the per-action longest-execution-time table.
    # This board's real word is "mstats" (tea.c machineStats()), not
    # "times" - that word doesn't exist here. Output has no declared
    # per-action limit, only the observed max, so ActionTiming.limit stays
    # None until real limits are sourced from somewhere and wired in - see
    # parse_timing_table in timing.py.
    timing_report_cmd: str = "mstats"
    # Word that zeroes the recorded maxima: "0stats" (tea.c
    # zeroMachineTimes()), not "times.clear".
    timing_reset_cmd: str = "0stats"
    # Units the instrument reports in, for readable failure messages.
    timing_units: str = "us"

    # ------------------------------------------------------------------
    # CRC
    # ------------------------------------------------------------------
    # TODO: set True once the text CRC format below matches the firmware.
    crc_enabled: bool = False
    # Separator between payload and CRC, e.g. "PAYLOAD *1A2B"
    crc_separator: str = "*"
    # Width of the hex CRC field.
    crc_digits: int = 4

    # ------------------------------------------------------------------
    # Timeouts (seconds)
    # ------------------------------------------------------------------
    # How long the transport fixture polls for the target to start
    # answering at all, before the real test session begins. Was declared
    # but never used - the fixture just did a fixed 0.3s drain and dove
    # straight into test traffic. Real hardware has shown an unresponsive
    # window of up to roughly a minute right after the port opens (see
    # conftest._wait_until_responsive); 5s wasn't close, and every command
    # sent into that window burned a full command_timeout looking like a
    # link failure instead of a one-time startup wait.
    connect_timeout: float = 120.0
    command_timeout: float = 2.0
    # Commands known to run long; name -> timeout override.
    command_timeout_overrides: dict[str, float] = field(default_factory=dict)


DEFAULT = TargetConfig()

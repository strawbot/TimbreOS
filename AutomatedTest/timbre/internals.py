"""
Static checks on the target's own bookkeeping: its action-name tables and
its stack canary.

These look at what the firmware says about itself, not at what a command
does, so they are the part of the suite that tells a port whether the inner
system is wired up correctly. A missing namedAction() leaves a bare hex
address where mstats/te should print a function name; a canary that is not
OK means something already ran off the end of its stack.

The parsing lives here, apart from the tests, so the end-of-run reports in
conftest.py and the named tests in tests/test_internals.py read the board's
output the same way - and so it can be unit-tested against sample output
without a board.
"""

from __future__ import annotations

import re
import subprocess
import sys

from .timing import parse_timing_table

# A bare hex address in a name column: what mstats/te print for an action
# with no namedAction() name (tea.c machineStats(): "(machine + 2) & ~3";
# printActionName()/action_key(): "(action + 1) & ~3" - equivalent for any
# real function pointer, since Thumb code addresses only ever land on a %4
# of 0 or 1, where both formulas round to the same value). Anchored to 4-8
# hex digits, zero-padded uppercase being the form these words print, e.g.
# "0000BA40".
#
# Known limit: a function that is genuinely named something made only of
# a-f/0-9 characters, 4 or more long ("dead", "face", "beef"), reads as an
# address. Rename it; the check is cheap enough to be worth that.
HEX_ACTION = re.compile(r"^[0-9A-Fa-f]{4,8}$")


def mstats_hex_names(report: str) -> list[str]:
    """Names in an mstats table that are addresses, not function names."""
    return [t.name for t in parse_timing_table(report) if HEX_ACTION.match(t.name)]


def te_hex_names(report: str) -> list[str]:
    """Same for te. Its rows look nothing like mstats's - "in "/"after ", a
    formatted delay, tabTo(17), then the name - so this is parsed on its own
    rather than reusing parse_timing_table. Deliberately anchored to lines
    that start with the words print_te() actually uses, rather than scanning
    the whole report for hex-looking tokens: a timing VALUE elsewhere in the
    text (e.g. "3250", or the "ticks:91999493" header) is valid hex too, and
    would false-match a blind scan.
    """
    names = []
    for line in report.splitlines():
        line = line.strip()
        if line.startswith("in ") or line.startswith("after "):
            tokens = line.split()
            if tokens and HEX_ACTION.match(tokens[-1]):
                names.append(tokens[-1])
    return names


def unnamed_actions(mstats_report: str, te_report: str) -> list[str]:
    """Every bare address across both reports, once each, sorted."""
    return sorted(set(mstats_hex_names(mstats_report) + te_hex_names(te_report)))


def resolve_unnamed_actions(mstats_report: str, te_report: str, cfg) -> str | None:
    """Run resolve_action.py on every bare hex address mstats/te printed.

    namedAction() is opt-in (see resolve_action.py's own docstring for why),
    so most actions never get a name - this turns "here's a hex address"
    into an actual function-and-line recommendation, automatically, rather
    than relying on a human to notice, remember the tool exists, and copy
    the address over by hand. None when there is nothing to resolve or no
    script to resolve with.
    """
    script = cfg.resolve_action_script
    if not script or not script.exists():
        return None
    addresses = unnamed_actions(mstats_report, te_report)
    if not addresses:
        return None
    result = subprocess.run(
        [sys.executable, str(script), *addresses],
        capture_output=True, text=True, timeout=15,
    )
    return result.stdout or result.stderr


# board_cli.c's show_sys() ends with a "stack:" line rendered by
# Robot/diagnostics/canary/canary.c stack_render():
#     stack:   OK  hwm=1008 /2048  B
# or WARNING (high-water mark exceeded _Min_Stack_Size), OVERFLOW (usage ran
# into BSS - the line then includes a hex dump of the incursion), or
# CRITICAL (consumed all free RAM).
CANARY_STATUSES = ("OK", "WARNING", "OVERFLOW", "CRITICAL")


def canary_stack_line(lines: list[str]) -> str | None:
    return next((l for l in lines if l.strip().startswith("stack:")), None)


def canary_stack_status(lines: list[str]) -> str | None:
    """The canary's verdict word, or None if there is no stack: line (or it
    carries no verdict) - show-sys changed shape, or this build has no
    canary."""
    line = canary_stack_line(lines)
    if line is None:
        return None
    parts = line.split()
    return parts[1] if len(parts) > 1 else None

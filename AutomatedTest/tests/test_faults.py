"""
Faults must leave the interpreter alive and in a known state.

Each case here is a crash that was reproduced on the host build (and, for
the stack ones, the same hazard as a board going silent): the CLI used to
keep running a word after its stack had been cleared or its arguments were
missing, and words like strlen/type then dereferenced whatever was left as
a pointer. The response is now a defined one - report the error, resetcli,
carry on - and these pin it so a regression shows up as a failed test
instead of a dead process or a wedged board.

"Alive" means: no timeout, an error was reported, and a plain `1 2 +` still
answers 3 afterwards.
"""

import pytest


def _alive(cli):
    # A bare number rather than `1 2 +`: it has to work on any target.
    cli.clear_stack()
    resp = cli.execute("7")
    assert resp.ok, f"interpreter did not answer after a fault: {resp.errors}"
    assert cli.stack().top == 7, "interpreter answered wrongly after a fault"
    cli.clear_stack()


def _real_firmware_only(request, what):
    """The fake target (--transport=host) is a protocol stand-in, not a model
    of the firmware: it has no stack-overflow detection, here-space bounds or
    input buffer, so there is nothing for these cases to test there."""
    if request.config.getoption("--transport") == "host":
        pytest.skip(f"fake_target.py does not model {what}")


def _need(cli, *words):
    missing = [w for w in words if w not in cli.commands()]
    if missing:
        pytest.skip(f"target has no {missing}")


# ----------------------------------------------------------------------
# Underflow: used to return 0 and let the word dereference it
# ----------------------------------------------------------------------
@pytest.mark.parametrize(
    "line",
    ["strlen", "5 type", "@", "c@", "s@", "!", "7 !", "c!", "dup", "swap", "+", "1 *"],
)
def test_underflow_reports_and_resets_instead_of_crashing(cli, line):
    _need(cli, *[w for w in line.split() if not w.lstrip("-").isdigit()])
    cli.clear_stack()
    resp = cli.execute(line)
    assert not resp.timed_out, f"{line!r} hung the interpreter"
    assert resp.errors, f"{line!r} on a short stack ran without reporting underflow"
    assert cli.depth() == 0, "the faulting line must end in a reset, not half-run"
    _alive(cli)


# ----------------------------------------------------------------------
# Overflow in the middle of an expression
# ----------------------------------------------------------------------
# Sent unframed on purpose. A framed command pushes three transient items for
# its own marker, so an overflow during framing happens inside the marker,
# the session sees a lost frame and quietly retries it, and the error never
# reaches the test. (And `.s` cannot report a depth above half the stack -
# the firmware reads that as underflow - so capacity cannot be read off it.)
def _raw(cli, text):
    return cli.s.raw(text + cli.cfg.tx_eol, quiet_for=0.05)


def _overflowed(lines) -> bool:
    return any("stack overflow" in line for line in lines)


def _stack_capacity(cli) -> int:
    """Items the parameter stack holds: push one at a time until it says so."""
    cli.reset()
    pushed = 0
    while not _overflowed(_raw(cli, "0")):
        pushed += 1
        assert pushed < 512, "parameter stack never reported overflow"
    cli.reset()  # the overflow reset it; this also re-syncs the link
    return pushed


def test_overflow_mid_expression_resets_instead_of_crashing(cli, request):
    """Overflow a couple of items into `"X" dup dup strlen type`.

    This is the original segfault: the overflow cleared the stack under the
    running expression, strlen then popped an empty stack, got 0, and
    called strlen on it.
    """
    _real_firmware_only(request, "a bounded parameter stack")
    _need(cli, "strlen", "type", "dup", "cr")
    capacity = _stack_capacity(cli)
    assert capacity >= 4, f"implausibly small stack: {capacity}"
    for _ in range(capacity - 2):
        assert not _overflowed(_raw(cli, "0"))
    # quote, dup, dup push three more: the last one overflows
    out = _raw(cli, '"X" dup dup strlen type cr')
    assert _overflowed(out), f"overflow mid-expression was not reported: {out}"
    cli.reset()
    _alive(cli)


# ----------------------------------------------------------------------
# here-space: allot can rewind (here - allot) but never leave it
# ----------------------------------------------------------------------
@pytest.mark.parametrize("count", ["-100000", "-1000000", "2000000000"])
def test_allot_out_of_range_is_rejected_and_here_is_unchanged(cli, request, count):
    """A count that would move here outside the dictionary used to be
    accepted (negative values wrapped, passed the upper-bound check, and
    moved here below the start of here-space); the next write at here then
    landed in whatever memory sat there."""
    _real_firmware_only(request, "here-space bounds")
    _need(cli, "allot", "here")
    cli.clear_stack()
    cli.execute("here")
    before = cli.stack().top
    cli.clear_stack()
    resp = cli.execute(f"{count} allot")
    assert not resp.timed_out, f"{count} allot hung the interpreter"
    assert resp.errors, f"{count} allot was accepted"
    cli.clear_stack()
    cli.execute("here")
    after = cli.stack().top
    cli.clear_stack()
    assert after == before, f"here moved from {before:#x} to {after:#x} on a rejected allot"
    # and something that writes at here still works
    resp = cli.execute("255 .h")
    assert resp.ok, resp.errors
    _alive(cli)


def test_allot_still_rewinds_what_it_allocated(cli):
    """The legitimate negative count: forgetting a string by allot()ing back."""
    _need(cli, "allot", "here")
    cli.clear_stack()
    cli.execute('here "abc" here - allot here')
    values = list(cli.stack().values)
    cli.clear_stack()
    assert len(values) == 2 and values[0] == values[1], (
        f"here before and after `\"abc\" here - allot` differ: {values}"
    )

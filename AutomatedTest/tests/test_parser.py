"""
Layer 1: the interpreter's own rules.

Left to right, one word at a time. A word is a command if it is in the
table, otherwise it is a number and gets pushed. Anything that is neither is
an error. These tests pin that contract, mostly through its negative paths,
which is where hand-written suites are always thinnest.
"""

import pytest

from tests.test_command_matrix import COMPILE_ONLY, DESTRUCTIVE, STACK_WORDS
from tests.test_stack_effects import PROBE_UNSAFE


# ----------------------------------------------------------------------
# Number parsing
# ----------------------------------------------------------------------
@pytest.mark.parametrize(
    "literal",
    ["0", "1", "-1", "255", "-2147483648", "2147483647", "4294967295"],
)
def test_numbers_push_one_item(cli, cfg, literal):
    cli.clear_stack()
    cli.execute(literal)
    assert cli.depth() == cfg.baseline_depth + 1, f"{literal!r} did not push exactly one"
    cli.clear_stack()


def test_multiple_numbers_push_in_order(cli, cfg):
    """Left to right, so the last number written is on top."""
    cli.clear_stack()
    cli.execute("11 22 33")
    assert cli.depth() == cfg.baseline_depth + 3
    cli.clear_stack()


@pytest.mark.parametrize(
    "word",
    ["notacommand", "12abc", "0x", "--5", "1.2.3", "+", "@@@"],
)
def test_unparseable_word_is_rejected(cli, word):
    """Neither a command nor a number, so it must error and not push."""
    cli.expect_error(word)
    cli.clear_stack()


def test_error_aborts_rest_of_line(cli, cfg):
    """A bad word must stop the line, not let later words run regardless.

    Matters for field debugging: a partially executed command line on a
    robot is worse than a rejected one.
    """
    cli.clear_stack()
    cli.expect_error(f"1 2 notacommand 3 4")
    depth = cli.depth()
    assert depth <= cfg.baseline_depth + 2, (
        f"words after the failing word still executed (depth {depth})"
    )
    cli.clear_stack()


# ----------------------------------------------------------------------
# Whitespace
# ----------------------------------------------------------------------
@pytest.mark.parametrize("gap", [" ", "  ", "     ", "\t", " \t "])
def test_separators_collapse(cli, cfg, gap):
    cli.clear_stack()
    cli.execute(gap.join(["1", "2", "3"]))
    assert cli.depth() == cfg.baseline_depth + 3
    cli.clear_stack()


def test_leading_and_trailing_space(cli, cfg):
    cli.clear_stack()
    cli.execute("   7   ")
    assert cli.depth() == cfg.baseline_depth + 1
    cli.clear_stack()


# ----------------------------------------------------------------------
# Parameter stack
# ----------------------------------------------------------------------
def test_clear_empties_the_stack(cli, cfg):
    cli.execute("1 2 3 4 5")
    cli.clear_stack()
    assert cli.depth() == cfg.baseline_depth


def test_stack_persists_across_lines(cli, cfg):
    """Documents the contract the stack guard exists to police."""
    cli.clear_stack()
    cli.execute("42")
    assert cli.depth() == cfg.baseline_depth + 1, "stack did not survive the line break"
    cli.clear_stack()


def test_underflow_is_reported_not_crashed(cli, commands, cfg):
    """Every command needing parameters must reject a short stack.

    Preloaded with marked values rather than run against an empty stack.
    An empty stack tells you only that the command errored; it cannot tell
    you whether the command checked depth before popping or after. Both
    report the error.

    Each command is given arity-1 sentinels: one short of what it needs,
    with a recognisable floor underneath - but the floor is not expected
    to survive. A stack boundary breach now triggers a full resetcli
    (cli.c's push()/pop()/checked-TOP fault path, see cli.c's
    stack_fault jmp_buf), not a surgical "only this command's own
    operands are lost" recovery: continuing to run the faulting word's
    remaining C code against a stack that doesn't have what it expects
    is what let `1 *` read a stale/garbage second operand silently
    instead of reporting underflow at all, and what let a genuine
    overflow-mid-expression segfault by handing a bad address to
    type()/strlen(). A full reset is the only response that's actually
    safe to keep running after. So the check here is that the sentinels
    are GONE, not intact - the whole stack, not just this command's
    declared arguments, was cleared.
    """
    checked = 0
    for name, spec in sorted(commands.items()):
        if spec.arity == 0 or spec.takes_text:
            continue
        if name in (cfg.clear_stack_cmd, cfg.stack_cmd):
            continue
        # A sentinel is a garbage value by design. For most commands that's
        # fine - the whole point is to prove the command never touches it.
        # But a memory/actuator word (!, c!, +b, ...) doesn't just fail on
        # a bad address or channel, it ACTS on it: writing through a
        # 0x5EED-class sentinel address hung the real board outright the
        # first time this sweep ran without this exclusion. See
        # test_stack_effects.PROBE_UNSAFE for the list and why.
        #
        # if/for/until/while are compile-only (see COMPILE_ONLY) and error
        # unconditionally when interpreted bare, regardless of what's on
        # the stack - not an underflow in the sense this sweep is checking.
        if name in PROBE_UNSAFE or name in DESTRUCTIVE or name in COMPILE_ONLY:
            continue

        cli.clear_stack()
        cli.push_sentinels(spec.arity - 1)
        resp = cli.execute(name)

        assert not resp.timed_out, f"{name!r} hung on stack underflow"
        assert resp.errors, (
            f"{name!r} accepted {spec.arity - 1} parameter(s) but declares "
            f"arity {spec.arity} ({spec.raw.strip()})"
        )
        # The real check: underflow must reset to a known-good state, not
        # continue running with whatever was left on the stack.
        assert cli.depth() == cfg.baseline_depth, (
            f"{name!r} on underflow: expected a full reset to depth "
            f"{cfg.baseline_depth}, found {cli.stack()}"
        )
        checked += 1

    cli.clear_stack()
    assert checked, "no commands with arity > 0 were discovered; check describe parsing"


def test_commands_do_not_reach_below_their_arguments(cli, commands, cfg):
    """With exactly the right arguments, the floor beneath must survive.

    Catches the opposite error from underflow: a command that pops one more
    value than it declared. Harmless-looking in isolation, because the extra
    value is usually there, and it silently eats the caller's data.
    """
    offenders = []
    for name, spec in sorted(commands.items()):
        if spec.arity == 0 or spec.takes_text:
            continue
        if name in DESTRUCTIVE or name in STACK_WORDS or name in PROBE_UNSAFE:
            continue
        # if/for/until/while are compile-only and error unconditionally
        # when interpreted bare - the "floor" they leave is whatever a
        # full resetcli leaves, not something the command declared.
        if name in COMPILE_ONLY:
            continue
        cli.clear_stack()
        marks = cli.push_sentinels(2)
        cli.execute(" ".join(["1"] * spec.arity))
        cli.execute(name)
        snap = cli.stack()
        found = snap.sentinels(cfg.sentinel_base)
        if found != marks:
            offenders.append(f"{name}: floor went from {len(marks)} to {len(found)} ({snap})")
    cli.clear_stack()
    assert not offenders, "commands that reached below their arguments:\n  " + "\n  ".join(offenders)


def test_stack_depth_bound(cli, cfg):
    """Pushing past capacity must error, not overflow into adjacent memory."""
    cli.clear_stack()
    for _ in range(64):
        cli.execute(" ".join(["1"] * 16))
        if cli.execute(cfg.stack_cmd).errors:
            break
    resp = cli.execute(cfg.stack_cmd)
    assert not resp.timed_out, "interpreter died while filling the parameter stack"
    cli.clear_stack()
    assert cli.depth() == cfg.baseline_depth


# ----------------------------------------------------------------------
# Multi-command lines
# ----------------------------------------------------------------------
def test_multiple_commands_execute_in_order(cli, cfg):
    """The property the whole framing scheme rests on.

    Uses the quote+type idiom rather than cfg.mark_cmd: on this board
    "echo" is a filesystem word (<name> <text...> appends to a file), not
    a print word, so building a print-two-words test out of it was
    silently writing real files to the target's flash on every run - see
    AutomatedTest/README or git history for how that was found.
    """
    resp = cli.execute('"FIRST" dup strlen type cr "SECOND" dup strlen type cr')
    text = resp.text
    assert "FIRST" in text and "SECOND" in text
    assert text.index("FIRST") < text.index("SECOND"), "words ran out of order"


def test_values_precede_their_command(cli, commands, cfg):
    """TODO: pin against a real arithmetic or accessor word.

    Pick a pure command with a known result and assert it here — this is the
    test that proves RPN ordering end to end.
    """
    pytest.skip("choose a pure command with a known result and assert it")

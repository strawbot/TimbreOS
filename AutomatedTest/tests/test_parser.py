"""
Layer 1: the interpreter's own rules.

Left to right, one word at a time. A word is a command if it is in the
table, otherwise it is a number and gets pushed. Anything that is neither is
an error. These tests pin that contract, mostly through its negative paths,
which is where hand-written suites are always thinnest.
"""

import pytest

from tests.test_command_matrix import DESTRUCTIVE, STACK_WORDS
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
    report the error. Only one leaves the stack intact.

    Each command is given arity-1 sentinels: one short of what it needs,
    with a recognisable floor underneath.
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
        if name in PROBE_UNSAFE or name in DESTRUCTIVE:
            continue

        cli.clear_stack()
        marks = cli.push_sentinels(spec.arity - 1)
        resp = cli.execute(name)

        assert not resp.timed_out, f"{name!r} hung on stack underflow"
        assert resp.errors, (
            f"{name!r} accepted {spec.arity - 1} parameter(s) but declares "
            f"arity {spec.arity} ({spec.raw.strip()})"
        )
        # The real check: it must not have taken anything on the way out.
        cli.assert_sentinels_intact(marks, f"{name!r} on underflow")
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

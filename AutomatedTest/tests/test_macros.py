"""
Macros and multi-line blocks.

Two separate benefits, worth keeping distinct.

Macros cut traffic. At 115200 a long test line is real wall-clock time, and
the sweep tests send thousands of them. Defining a sequence once and calling
it by name collapses that, and turns an unreadable wall of repeated words
into one named step.

Multi-line blocks cut framing overhead and keep a setup sequence readable as
one unit. Since the end marker is already sent on its own line, extending
that to N lines costs nothing structurally.

Both need cleanup discipline. A macro that outlives its test makes the next
test depend on execution order, which is the hardest kind of suite bug to
find.
"""

import pytest

from timbre import ProtocolError


# ----------------------------------------------------------------------
# Multi-line blocks
# ----------------------------------------------------------------------
def test_block_runs_every_line(cli, session, cfg):
    """Uses the quote+type idiom rather than cfg.mark_cmd: on this board
    "echo" is a filesystem word (<name> <text...> appends to a file), not
    a print word, so this used to silently write real files to the
    target's flash on every run.
    """
    resp = session.execute_block([
        '"ONE" dup strlen type cr',
        '"TWO" dup strlen type cr',
        '"THREE" dup strlen type cr',
    ])
    assert resp.ok, f"block failed: {resp.lines}"
    text = resp.text
    assert text.index("ONE") < text.index("TWO") < text.index("THREE")


def test_block_state_carries_between_lines(cli, session, cfg):
    """The stack persists across lines, so a block is one continuous step."""
    cli.clear_stack()
    resp = session.execute_block(["1 2", "3 4", cfg.stack_cmd])
    assert resp.ok
    assert cli.stack().depth == 4, "stack did not carry across lines in the block"
    cli.clear_stack()


def test_block_reports_an_error_from_any_line(cli, session, cfg):
    """A failure on line 2 must not be hidden by lines 1 and 3 succeeding."""
    resp = session.execute_block([
        '"BEFORE" dup strlen type cr',
        "definitely_not_a_command_xyzzy",
        '"AFTER" dup strlen type cr',
    ])
    assert resp.errors, f"error in the middle of a block went unreported: {resp.lines}"
    cli.clear_stack()


def test_oversized_block_line_is_refused_before_sending(session, cfg):
    """Caught by the pre-flight check, not discovered as a timeout."""
    with pytest.raises(ProtocolError, match="input buffer"):
        session.execute_block(["0 " * cfg.max_line_length])


# ----------------------------------------------------------------------
# Macros
# ----------------------------------------------------------------------
def test_macro_defines_and_runs(cli, cfg):
    with cli.macro("th_push3", "1 2 3"):
        cli.clear_stack()
        cli.execute("th_push3")
        assert cli.stack().depth == 3, "macro body did not execute"
        cli.clear_stack()


def test_macro_is_forgotten_after_use(cli, cfg):
    """Leaked macros make tests depend on execution order."""
    if not cfg.macro_forget_cmd:
        pytest.skip("target cannot forget macros; see config.macro_forget_cmd")
    with cli.macro("th_temp", "1"):
        pass
    assert "th_temp" not in cli.word_names(), (
        "macro survived its context manager; later tests may now see it"
    )


def test_macro_appears_in_the_word_list(cli):
    """Macros are discoverable, which means the coverage gate sees them too.

    Worth knowing: a macro left behind will fail the coverage gate as an
    untested command. That is the gate working, not a false positive.
    """
    with cli.macro("th_visible", "1 2"):
        assert "th_visible" in cli.word_names(refresh=True)
    cli.clear_stack()


def test_macro_reduces_traffic_for_repeated_setup(cli, cfg):
    """The practical payoff, measured rather than asserted.

    Not a pass/fail threshold: on the host build both paths are fast and
    the difference is noise. Run it on hardware to see the real saving
    before deciding whether to macro-ise a sweep.
    """
    setup = "1 2 3 4 5 6 7 8"
    reps = 10

    cli.clear_stack()
    inline = 0.0
    for _ in range(reps):
        inline += cli.execute(setup).elapsed
        cli.clear_stack()

    with cli.macro("th_setup", setup):
        macro = 0.0
        for _ in range(reps):
            macro += cli.execute("th_setup").elapsed
            cli.clear_stack()

    chars_inline = (len(setup) + 1) * reps
    chars_macro = (len("th_setup") + 1) * reps
    print(
        f"\n  inline: {inline * 1000:.1f}ms, {chars_inline} chars"
        f"\n  macro:  {macro * 1000:.1f}ms, {chars_macro} chars"
        f"\n  at {cfg.baud} baud that is ~"
        f"{(chars_inline - chars_macro) * 10 / cfg.baud * 1000:.1f}ms of "
        "line time saved"
    )
    cli.clear_stack()

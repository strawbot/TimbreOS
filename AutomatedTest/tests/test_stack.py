"""
Layer 1: the parameter stack, and the parser that reads it.

`test_stack_parser_agrees_with_target` comes first for a reason. Every stack
assertion in the suite is only as good as the `.s` parser, and a parser that
misreads the format while still producing a plausible snapshot would make
all of them meaningless while reporting green. So it is pinned against known
values pushed onto the real target rather than assumed from a format string.
"""

import pytest

from timbre import StackParseError, is_sentinel, parse_stack_output, sentinel


# ----------------------------------------------------------------------
# Display order: settled by experiment, not by reading comments
# ----------------------------------------------------------------------
def test_stack_display_order(cli, cfg, session):
    """Which end of the printed list is the top of the stack?

    Getting this wrong produces no error. It silently inverts every stack
    assertion in the suite: `dup` still passes because its result is
    symmetric, while `swap` and `over` look broken, and the conformance
    sweep reports confident nonsense about correct firmware.

    Reading it off the stack comments is circular - the comments are what
    the conformance tests are trying to verify. So this asks the target
    directly: push three distinct values, remove the top one, and see which
    end of the display lost a value. No convention assumed.

        11 22 33 drop  ->  "11 22"   rightmost was the top (bottom_first)
                       ->  "22 11"   leftmost was the top  (top_first)
    """
    cli.reset()
    try:
        _check_display_order(cli, cfg)
    finally:
        # Reset here, not at the end. If the assertion below fires the stack
        # is left loaded, the teardown guard fires too, and the actual
        # diagnosis gets buried under a second unrelated failure.
        cli.reset()


def _check_display_order(cli, cfg):
    resp = cli.execute("11 22 33 drop")
    assert not resp.errors, f"setup failed: {resp.errors}"

    raw = cli.execute(cfg.stack_cmd)
    assert raw.ok, f"could not read the stack: {raw}"

    from timbre.stack import _ints

    nums = _ints(raw.text.replace(":", " "))
    values = nums[1:] if cfg.stack_format == "count_first" else nums
    values = [v for v in values if v in (11, 22, 33)]

    assert len(values) == 2, (
        f"expected two values left after drop, got {values} from {raw.text!r}. "
        f"Check config.stack_format (currently {cfg.stack_format!r})."
    )

    if values == [11, 22]:
        observed = "bottom_first"
    elif values == [22, 11]:
        observed = "top_first"
    else:
        pytest.fail(
            f"`drop` removed {set((11, 22, 33)) - set(values)} instead of the "
            f"top value. Stack reads {values} from {raw.text!r}; either drop "
            "is not removing the top or the display is not a simple ordering."
        )

    assert observed == cfg.stack_display_order, (
        f"the target displays the stack {observed}, but config says "
        f"{cfg.stack_display_order!r}. Change config.stack_display_order to "
        f"{observed!r}. Until then every stack assertion in this suite is "
        "inverted, and swap/over will appear broken while dup passes."
    )


def test_display_order_matches_the_documented_examples(cli, cfg):
    """Reproduces the two transcripts the ordering was settled from.

        ar: 1 2 3 over .s   ->  4 stack items: 1 2 3 2
        ar: 1 2 3 swap .s   ->  3 stack items: 1 3 2

    Kept as a regression: if `.s` formatting or ordering ever changes,
    this fails with a directly comparable transcript rather than as a
    puzzling conformance mismatch somewhere else.
    """
    try:
        cli.reset()
        cli.execute("1 2 3 over")
        assert cli.stack().values == (1, 2, 3, 2), "`1 2 3 over` should leave 1 2 3 2"

        cli.reset()
        cli.execute("1 2 3 swap")
        assert cli.stack().values == (1, 3, 2), "`1 2 3 swap` should leave 1 3 2"
    finally:
        cli.reset()


# ----------------------------------------------------------------------
# Parser self-validation
# ----------------------------------------------------------------------
def test_stack_parser_agrees_with_target(cli, cfg):
    """Push known values, read them back, compare exactly.

    If this fails, set config.stack_format to match the real `.s` output
    before trusting anything else in this file.
    """
    cli.clear_stack()
    expected = (11, 22, 33, 44)
    cli.execute(" ".join(str(v) for v in expected))

    snap = cli.stack()
    assert snap.depth == len(expected), (
        f"`{cfg.stack_cmd}` reported depth {snap.depth} after pushing "
        f"{len(expected)} values. config.stack_format={cfg.stack_format!r} "
        "probably does not match the target."
    )
    assert snap.values == expected, (
        f"values came back as {snap.values}, expected {expected}. "
        "If the order is reversed, `.s` prints top-first and the parser "
        "needs to reverse it."
    )
    cli.clear_stack()


def test_empty_stack_parses(cli):
    cli.clear_stack()
    snap = cli.stack()
    assert snap.depth == 0
    assert snap.values == ()


@pytest.mark.parametrize(
    "text,fmt,depth,values",
    [
        ("4 stack items: 1 2 3 2", "count_first", 4, (1, 2, 3, 2)),
        ("0 stack items:", "count_first", 0, ()),
        ("<3> 11 22 33", "bracket", 3, (11, 22, 33)),
        ("<0>", "bracket", 0, ()),
        ("11 22 33", "values_only", 3, (11, 22, 33)),
    ],
)
def test_parser_formats(text, fmt, depth, values):
    snap = parse_stack_output(text, fmt)
    assert (snap.depth, snap.values) == (depth, values)


def test_parser_normalises_top_first_output():
    """However the target prints it, index 0 is the bottom after parsing."""
    bottom = parse_stack_output("3 stack items: 1 3 2", "count_first", "bottom_first")
    top = parse_stack_output("3 stack items: 2 3 1", "count_first", "top_first")
    assert bottom.values == top.values == (1, 3, 2)
    assert bottom.top == top.top == 2


def test_parser_rejects_inconsistent_output():
    """Depth and value count disagreeing must raise, never be smoothed over."""
    with pytest.raises(StackParseError):
        parse_stack_output("<5> 11 22", "bracket")


def test_parser_rejects_unrecognised_output():
    with pytest.raises(StackParseError):
        parse_stack_output("Stack is empty", "bracket")


# ----------------------------------------------------------------------
# Sentinels
# ----------------------------------------------------------------------
def test_sentinels_are_distinguishable(cfg):
    marks = [sentinel(i, cfg.sentinel_base) for i in range(1, 5)]
    assert len(set(marks)) == 4, "sentinels must be distinct to show ordering"
    for m in marks:
        assert is_sentinel(m, cfg.sentinel_base)
    for ordinary in (0, 1, -1, 42, 0xFFFF, 1000000):
        assert not is_sentinel(ordinary, cfg.sentinel_base), (
            f"{ordinary} looks like a sentinel; choose a different "
            "config.sentinel_base or the tests will report false failures"
        )


def test_sentinel_floor_survives_a_well_behaved_command(cli, cfg):
    """The scaffolding itself works: a floor laid down is a floor read back."""
    with cli.sentinel_floor(3, context="self-check") as marks:
        cli.execute(cfg.stack_cmd)
        snap = cli.stack()
        assert snap.sentinels(cfg.sentinel_base) == marks


def test_sentinel_floor_detects_a_disturbance(cli, cfg):
    """Prove the check fires, so a passing floor test means something.

    A guard that never triggers buys confidence it has not earned.
    """
    from timbre import StackError

    cli.clear_stack()
    marks = cli.push_sentinels(3)
    cli.execute(cfg.clear_stack_cmd)  # stand-in for a command that over-pops
    with pytest.raises(StackError, match="floor disturbed"):
        cli.assert_sentinels_intact(marks, "deliberate disturbance")
    cli.clear_stack()


def test_sentinels_show_consumption_order(cli, cfg, commands):
    """Distinct sentinels reveal WHICH values a command took, not just how many.

    TODO: point at a real command with a known stack effect. With sentinels
    as its arguments you can read the result and confirm RPN ordering
    end to end, rather than inferring it from depth alone.
    """
    pytest.skip("choose a command whose result reveals its argument order")

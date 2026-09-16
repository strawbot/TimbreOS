"""
`resetcli` — the isolation primitive.

The fixtures call this between every test, so the suite's independence rests
entirely on it doing what it claims. If reset leaves a nap pending or a macro
defined, tests start depending on execution order, and that is the hardest
class of suite bug to track down: everything passes alone and fails together,
or passes in CI and fails locally because the order differs.

So each of its five documented effects gets its own test, and each one sets
up the state it claims to clear rather than assuming it was there.

Reset also *stops interpreting* and *resets the input parser*, which makes it
hostile to the harness's own framing. It is sent unframed and followed by a
resync. The framing tests below pin that.
"""

import pytest


def test_reset_empties_the_parameter_stack(cli, cfg):
    cli.execute("1 2 3 4 5")
    assert cli.stack().depth == 5, "setup failed; nothing to reset"
    cli.reset()
    assert cli.stack().depth == cfg.baseline_depth


def test_reset_removes_macros(cli, cfg):
    """Leaked macros are why reset beats clearing the stack."""
    cli.execute(f"{cfg.macro_begin_cmd} th_survivor 1 2 3 {cfg.macro_end_cmd}")
    assert "th_survivor" in cli.word_names(), "setup failed; macro was not defined"
    cli.reset()
    assert "th_survivor" not in cli.word_names(), (
        "macro survived resetcli; every later test can now see it"
    )


@pytest.mark.hw
def test_reset_cancels_pending_naps(cli, cfg):
    """A nap outliving its test fires during an unrelated one.

    TODO: replace the nap count query with whatever TimbreOS exposes.
    Hardware-marked because a nap is a real scheduled event.
    """
    resp = cli.execute("1000 nap")
    if resp.errors:
        pytest.skip("no nap command exposed under that name")
    cli.reset()
    after = cli.execute("naps")
    assert after.ok, "no way to observe pending naps; add one or drop this test"
    assert "0" in after.text, f"naps still pending after reset: {after.text!r}"


def test_reset_restores_the_input_parser(cli, cfg, session):
    """Reset must recover the parser from mid-stream, not just from idle.

    Left partway through consuming text, a parser that is not reset will
    swallow the next command as an argument. That failure is invisible: the
    command simply never runs, and the test times out somewhere else.
    """
    # A text-consuming word with no text yet. Uses describe_cmd ("help"),
    # not mark_cmd ("echo"): on this board echo is a filesystem word, so
    # leaving it mid-argument is one command away from creating a file.
    session.raw(f"{cfg.describe_cmd}")
    cli.reset()
    assert cli.stack().depth == cfg.baseline_depth
    assert cli.execute(cfg.stack_cmd).ok, "parser did not recover after reset"


def test_reset_stops_interpreting_the_rest_of_the_line(cli, cfg, session):
    """Documented behaviour, and the reason reset is never sent framed.

    Words after resetcli on the same line are discarded. If the harness
    framed it, the closing marker would go with them.
    """
    session.raw(f"{cfg.reset_cmd} 111 222 333{cfg.tx_eol}", quiet_for=cfg.reset_settle)
    session.resync()
    assert cli.stack().depth == cfg.baseline_depth, (
        "values after resetcli were interpreted; either the documented "
        "behaviour changed or reset is not stopping the line"
    )


def test_framing_survives_a_reset(cli, cfg):
    """Normal framed commands work immediately afterwards.

    Worth pinning: reset discards buffered input, so a naive implementation
    could eat the harness's own end marker and leave every subsequent
    command reporting a timeout.
    """
    cli.reset()
    for _ in range(5):
        resp = cli.execute(cfg.stack_cmd)
        assert resp.ok and not resp.frame_lost, f"framing broken after reset: {resp}"


def test_reset_is_idempotent(cli, cfg):
    """Back-to-back resets must be harmless; the fixtures issue a lot of them."""
    for _ in range(5):
        cli.reset()
    assert cli.execute(cfg.stack_cmd).ok


def test_reset_from_a_corrupted_state(cli, cfg, session):
    """The case that matters in the field: recovery from actual garbage."""
    session.raw(bytes(range(1, 32)).decode("latin-1") + cfg.tx_eol)
    cli.reset()
    assert cli.execute(cfg.stack_cmd).ok, "resetcli could not recover the interpreter"

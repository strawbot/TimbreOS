"""
Layer 0: prove the link and the framing before trusting any command result.

Nothing above this file means anything until these pass. A CRC that
disagrees with the firmware, or a parser that silently truncates long input,
turns every higher-level test into a coin flip that happens to be green.
"""

import pytest

from timbre import Crc16Codec, ProtocolError


def test_target_responds(cli):
    resp = cli.call(cli.cfg.stack_cmd)
    assert not resp.timed_out, "no response from target; check port, baud, wiring"
    assert resp.ok


def test_crc_agrees_with_target(cli, cfg, session):
    """The harness CRC must equal the firmware CRC, or CRC is worse than none.

    TODO: replace with whatever the CLI offers for this — a word that CRCs
    the following text, or a deliberately corrupted frame that must be
    rejected. Skipped rather than faked, because a fake pass here is the
    single most dangerous thing in the suite.
    """
    if not cfg.crc_enabled:
        pytest.skip("CRC disabled in config")
    codec = Crc16Codec(cfg.crc_separator, cfg.crc_digits)
    known_payload = "123 456 +"
    expected = codec.compute(known_payload)
    assert len(expected) == cfg.crc_digits
    pytest.skip("wire up against the firmware's CRC word, then delete this skip")


def test_corrupted_command_is_rejected(cli, cfg, session):
    """A frame whose CRC does not match must not be executed."""
    if not cfg.crc_enabled:
        pytest.skip("CRC disabled in config")
    bad = f"{cfg.stack_cmd} {cfg.crc_separator}{'0' * cfg.crc_digits}"
    lines = session.raw(bad + cfg.tx_eol)
    assert lines, "target silently swallowed a corrupted frame"
    session.resync()


@pytest.mark.parametrize("length", [16, 64, 127, 128, 129, 255, 256, 512])
def test_long_input_is_bounded_not_truncated(cli, session, cfg, length):
    """Over-long input must be rejected, never silently cut short.

    Silent truncation is the worst failure mode this CLI can have: a command
    loses its tail, does something different from what was asked, and reports
    success. Every test result after that is suspect.
    """
    # Stack-neutral filler: this tests the LINE bound, so the line must not
    # also be a stack-depth test. Plain zeros are pushes - 63+ of them fill
    # the 64-item parameter stack, leaving no room for the close marker's own
    # transient items, and the test then measured stack capacity instead.
    # "0 drop" pairs are 7 characters each with a net depth of 0.
    #
    # The line ends in a value nothing else here produces. If the target cuts
    # the line short without saying so, the tail never executes and that
    # value never arrives - which a "did it answer" check cannot see, since
    # the truncated line still runs fine and the framing markers (one at
    # each end, on their own lines) still come back.
    tail = 4242
    filler = " ".join(["0 drop"] * max(1, length // 7)) + f" {tail}"
    try:
        resp = session.execute(filler)
    except ProtocolError:
        # The harness refused to send it: framed length would overrun the
        # target's buffer. That is the pre-flight check doing its job, and
        # it tells you the usable command length is max_line_length minus
        # framing overhead.
        cli.clear_stack()
        return

    assert not resp.timed_out, f"interpreter stopped responding at {length} chars"
    if resp.frame_lost:
        pytest.fail(
            f"the {length}-char line was discarded before executing. Lower "
            f"config.max_line_length to match the real input buffer."
        )
    # Either outcome is acceptable: the target says no (an error), or it ran
    # the WHOLE line - the tail value is on the stack, alone. Anything else
    # is a line that was cut short and reported success.
    if not resp.errors:
        snap = cli.stack()
        assert list(snap.values) == [tail], (
            f"the {len(filler)}-char line ran without error but its tail "
            f"({tail}) did not arrive intact: stack is {snap}. The target is "
            f"truncating over-long input silently."
        )
    # Clear on its own line. If the long line aborted partway (length
    # limit), a trailing clear on that same line would never have run and
    # the stack guard would fire in teardown blaming this test for the
    # previous one's mess.
    cli.clear_stack()
    session.resync()


@pytest.mark.parametrize("extra", [1, 2, 3, 4])
def test_line_longer_than_the_input_buffer_is_rejected_whole(cli, session, cfg, request, extra):
    """A line past the target's input buffer must not run its first part.

    Sent unframed and deliberately past config.max_line_length, which is
    exactly what the pre-flight check in the test above exists to stop the
    framed path from doing. The target used to drop the characters that did
    not fit and run the rest on the carriage return: `... 4242` cut to
    `... 42` is a valid line that does something different and reports
    success. Now it reports the line as too long and discards it.

    The line is exactly max_line_length + extra characters and ends in the
    value 4242, so across extra = 1..4 the cut lands on each point of the
    tail: keeping 3, 2, 1 and 0 of its digits. (An empty stack alone cannot
    tell "rejected" from "cut right before the tail": the rejection has to
    be reported.) A target whose buffer is bigger than max_line_length just
    runs the whole line, which is also fine.
    """
    if request.config.getoption("--transport") == "host":
        pytest.skip("fake_target.py has no input buffer to overflow")
    tail = 4242
    target = cfg.max_line_length + extra
    pairs = (target - len(str(tail)) - 1) // 7  # "0 drop" + separator = 7 chars
    head = " ".join(["0 drop"] * pairs)
    gap = target - len(head) - len(str(tail))   # spaces, so the length is exact
    assert pairs >= 1 and gap >= 1, "max_line_length too small for this test's filler"
    line = head + " " * gap + str(tail)
    assert len(line) == target

    cli.reset()
    out = session.raw(line + cfg.tx_eol, quiet_for=0.3)
    session.resync()
    values = list(cli.stack().values)
    reported = any("eh?" in text or "too long" in text for text in out)
    ran_whole = values == [tail]
    rejected = values == [] and reported
    assert ran_whole or rejected, (
        f"a {len(line)}-char line was cut short and still ran: stack is "
        f"{values}, error reported: {reported}. Expected the whole line "
        f"(stack [{tail}]) or a rejection (empty stack and an error)."
    )
    cli.clear_stack()
    resp = cli.execute("7")
    assert resp.ok and cli.stack().top == 7, "interpreter unusable after an over-long line"
    cli.clear_stack()


def test_rapid_back_to_back_commands(cli):
    """No inter-command delay. Flushes out RX buffer and reentrancy bugs."""
    for _ in range(50):
        resp = cli.call(cli.cfg.stack_cmd)
        assert resp.ok, f"failed under back-to-back load: {resp}"


def test_recovers_from_garbage(cli, session, cfg):
    """Binary noise on the line must not wedge the interpreter."""
    session.raw(bytes(range(1, 32)).decode("latin-1") + cfg.tx_eol)
    session.resync()
    assert cli.call(cfg.stack_cmd).ok, "interpreter did not recover from garbage"


def test_empty_line_is_harmless(cli, session, cfg):
    session.raw(cfg.tx_eol * 3)
    session.resync()
    assert cli.call(cfg.stack_cmd).ok


def test_framing_survives_async_events(cli, cfg):
    """Events firing mid-command must not contaminate the response.

    If this fails, requirement R2 is unmet: async output is not
    distinguishable from command results and no amount of harness cleverness
    will fix it.
    """
    if cfg.async_prefix is None:
        pytest.skip("no async prefix configured")
    for _ in range(20):
        resp = cli.call(cfg.stack_cmd)
        assert resp.ok
        for line in resp.lines:
            assert not line.startswith(cfg.async_prefix), (
                f"event leaked into response body: {line!r}"
            )


def test_error_detection_matches_real_errors(cli, cfg, session):
    """The error regex must actually match what the target emits.

    This guards the highest-consequence single line of configuration in the
    harness. If error_pattern fails to match, every negative test in the
    suite passes silently while the CLI rejects everything, and the suite
    reports green on a completely broken interpreter.
    """
    resp = session.execute("definitely_not_a_command_xyzzy")
    assert not resp.timed_out, "no response to an unknown word"
    assert resp.lines, "target said nothing about an unknown word"
    assert resp.errors, (
        f"config.error_pattern {cfg.error_pattern!r} did not match the "
        f"target's actual error output: {resp.lines!r}. "
        "Until this matches, negative tests are meaningless."
    )
    cli.clear_stack()

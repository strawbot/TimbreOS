"""
Does the target's inner system report itself correctly?

These are porting checks: they read what the firmware says about its own
bookkeeping instead of exercising a command.

  * mstats and te name every action. An action with no namedAction() shows
    up as a bare hex address, which means a missing namedAction() in the
    source - the same check for both words.
  * the stack canary reports OK (show-sys), i.e. nothing has run off the
    end of the stack.

Two kinds of test live here. The parser tests run against sample output
(taken from real board runs) on any target, so the checks themselves are
verified without a board. The live tests read the real words and skip on a
target that doesn't have them - the host builds have no te, mstats or
canary, so these are for real boards. They are marked `internals` and run
last, because mstats only lists actions that have already run.

A board still being brought up can run with --internals-warn to see the same
findings as warnings.
"""

import warnings

import pytest

from timbre.internals import (
    canary_stack_status,
    mstats_hex_names,
    resolve_unnamed_actions,
    te_hex_names,
    unnamed_actions,
)

# ----------------------------------------------------------------------
# Sample output, shaped like the real thing (tea.c machineStats() and
# print_te(), board_cli.c show_sys()).
# ----------------------------------------------------------------------
MSTATS_NAMED = """\
 mstats: 3
    1496 us  cli
      25 us  dotPrompt
    2208 ns  run_cli
latency in us:     min     max     avg
in(secs(1),)     1000000 -1000000     0
later()          1000000 -1000000     0
"""

MSTATS_WITH_ADDRESS = """\
 mstats: 3
    1496 us  cli
      17 us  0000BA40
    2208 ns  run_cli
latency in us:     min     max     avg
"""

TE_NAMED = """\
  ticks/S:46875   ticks:91999493   UTC:1789515981
in 428 msec      blink_leds
after 5 secs     measure_latency
max overdue: 1   te min: 77
"""

TE_WITH_ADDRESS = """\
  ticks/S:46875   ticks:91999493   UTC:1789515981
in 428 msec      blink_leds
after 3 secs     00008E44
after 5 secs     measure_latency
max overdue: 1   te min: 77
"""


# ----------------------------------------------------------------------
# The checks themselves
# ----------------------------------------------------------------------
def test_mstats_names_pass_when_every_action_is_named():
    assert mstats_hex_names(MSTATS_NAMED) == []


def test_mstats_address_is_found():
    assert mstats_hex_names(MSTATS_WITH_ADDRESS) == ["0000BA40"]


def test_te_names_pass_when_every_action_is_named():
    assert te_hex_names(TE_NAMED) == []


def test_te_address_is_found():
    assert te_hex_names(TE_WITH_ADDRESS) == ["00008E44"]


def test_te_header_and_footer_numbers_are_not_mistaken_for_addresses():
    """ticks:91999493 and 'te min: 77' are hex-looking text, not action
    rows; only the in/after rows have a name column."""
    assert te_hex_names(TE_NAMED) == []
    assert te_hex_names("ticks:91999493 max overdue: 1234 te min: 5678\n") == []


def test_unnamed_actions_merges_both_words_once_each():
    both = unnamed_actions(
        MSTATS_WITH_ADDRESS + "after 1 secs     0000BA40\n", TE_WITH_ADDRESS
    )
    assert both == ["00008E44", "0000BA40"]


@pytest.mark.parametrize(
    "line, status",
    [
        ("stack:   OK  hwm=1008 /2048  B", "OK"),
        ("stack:   WARNING  hwm=2100 /2048  B", "WARNING"),
        ("stack:   OVERFLOW  incursion 00 11 22 33", "OVERFLOW"),
        ("stack:   CRITICAL", "CRITICAL"),
    ],
)
def test_canary_status_is_read_from_the_stack_line(line, status):
    lines = ["clk_sys: 125000000", "uptime: 12 s", line]
    assert canary_stack_status(lines) == status


def test_canary_status_is_none_without_a_stack_line():
    assert canary_stack_status(["clk_sys: 125000000", "uptime: 12 s"]) is None
    assert canary_stack_status(["stack:"]) is None


# ----------------------------------------------------------------------
# The live checks
# ----------------------------------------------------------------------
@pytest.fixture
def enforce(request):
    """Fail on a finding - or, under --internals-warn, say it and carry on."""
    warn_only = request.config.getoption("--internals-warn")

    def check(ok, message):
        if ok:
            return
        if warn_only:
            warnings.warn(message, UserWarning, stacklevel=2)
        else:
            pytest.fail(message, pytrace=False)

    return check


def _need(cli, word):
    if word not in cli.commands():
        pytest.skip(f"target has no {word!r}")


def _read(cli, cfg, word):
    resp = cli.execute(word, timeout=max(cfg.command_timeout, 5.0))
    assert not resp.timed_out, f"{word!r} did not answer"
    return resp


def _unnamed_message(where, names, mstats, te, cfg):
    message = (
        f"{where} prints {len(names)} action(s) as a bare address instead of a "
        f"name: {names}.\n"
        "Each one is missing a namedAction() for its action in the source."
    )
    hint = resolve_unnamed_actions(mstats, te, cfg)
    return message + (f"\n\n{hint}" if hint else "")


@pytest.mark.internals
def test_mstats_names_every_action(cli, cfg, enforce):
    _need(cli, cfg.timing_report_cmd)
    report = _read(cli, cfg, cfg.timing_report_cmd).text
    names = mstats_hex_names(report)
    enforce(not names, _unnamed_message(cfg.timing_report_cmd, names, report, "", cfg))


@pytest.mark.internals
def test_te_names_every_action(cli, cfg, enforce):
    _need(cli, cfg.time_events_cmd)
    report = _read(cli, cfg, cfg.time_events_cmd).text
    names = te_hex_names(report)
    enforce(not names, _unnamed_message(cfg.time_events_cmd, names, "", report, cfg))


@pytest.mark.internals
def test_canary_reports_stack_health_ok(cli, cfg, enforce):
    """A canary that is not OK means something already ran off the end of its
    stack - not necessarily the test that happens to run last."""
    _need(cli, cfg.show_sys_cmd)
    resp = _read(cli, cfg, cfg.show_sys_cmd)
    status = canary_stack_status(resp.lines)
    enforce(
        status is not None,
        f"{cfg.show_sys_cmd} printed no 'stack:' line: the canary is not wired "
        f"in, or its report changed shape. Got: {resp.lines}",
    )
    enforce(
        status is None or status == "OK",
        f"canary reports stack health {status}, not OK.\n"
        "WARNING: the high-water mark exceeded _Min_Stack_Size. OVERFLOW: usage "
        "ran into BSS. CRITICAL: it consumed all free RAM. "
        "See Robot/diagnostics/canary/canary.c stack_render().\n"
        + next((l for l in resp.lines if l.strip().startswith("stack:")), ""),
    )

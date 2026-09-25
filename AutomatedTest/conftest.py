"""
Fixtures, the automatic stack guard, and a session-wide timing snapshot.

stack_guard catches a failure mode that's invisible at the point where it's
caused and only shows up later somewhere unrelated - exactly the kind of
bug a human-written test suite never covers: a command that leaves a value
on the parameter stack. It runs around every test, with no per-test
authoring cost.

timing_snapshot is not a per-test guard: mstats/0stats measure the
C-level TEA action stats (the state machines), a session-wide concern
independent of which CLI command any given test happens to exercise. It
resets once at the start of the run and reports once at the end - see its
own docstring below.

Usage:
    pytest --transport=host            # fast, no board - tools/fake_target.py,
                                        # a hand-written Python reimplementation
    pytest --transport=tea-host        # fast, no board - the real firmware
                                        # source (cli.c/float.c/printers.c)
                                        # compiled and run natively via
                                        # Tea-host (see tools/tea_host_cli)
    pytest --transport=serial          # real hardware
    pytest --transport=serial -m hw    # hardware-only tests
"""

from __future__ import annotations

import pathlib
import re
import subprocess
import sys

import pytest

from timbre import (
    Cli,
    HostTransport,
    SerialTransport,
    Session,
    TargetConfig,
    TimingInstrument,
    build_codec,
    wait_until_responsive,
)
from timbre.timing import parse_timing_table


def pytest_addoption(parser):
    parser.addoption(
        "--transport", default="host", choices=["host", "serial", "tea-host", "tea-host-double"]
    )
    parser.addoption("--port", default=None, help="override serial port")
    parser.addoption("--no-timing-guard", action="store_true")
    parser.addoption("--no-stack-guard", action="store_true")
    parser.addoption("--prove-guards", action="store_true",
                     help="run the guard self-check tests, which are meant to fail")
    parser.addoption(
        "--session-log", default=None,
        help="append every line sent and received, in order, to this file - "
             "the run's conversation with the target, readable as a transcript",
    )


def pytest_configure(config):
    config.addinivalue_line("markers", "hw: requires real hardware")
    config.addinivalue_line("markers", "slow: long running")
    config.addinivalue_line("markers", "destructive: changes persistent target state")


def pytest_collection_modifyitems(config, items):
    if config.getoption("--transport") not in ("host", "tea-host", "tea-host-double"):
        return
    skip_hw = pytest.mark.skip(reason="needs --transport=serial")
    for item in items:
        if "hw" in item.keywords:
            item.add_marker(skip_hw)


@pytest.fixture(scope="session")
def cfg(request) -> TargetConfig:
    config = TargetConfig()
    if request.config.getoption("--port"):
        config.port = request.config.getoption("--port")
    return config


_TEA_HOST_CLI_DIR = pathlib.Path(__file__).parent / "tools" / "tea_host_cli"


def _tea_host_binary(double: bool = False) -> list[str]:
    """Build (if needed) and return the argv for the real-firmware host
    binary - see tools/tea_host_cli/Makefile. `make` no-ops if the binary
    is already newer than its sources, so this is cheap on repeat runs.
    double selects the 64 bit float build (-DFLOAT_DOUBLE).
    """
    binary = _TEA_HOST_CLI_DIR / ("tea_host_cli_double" if double else "tea_host_cli")
    result = subprocess.run(
        ["make", "double" if double else "all"], cwd=_TEA_HOST_CLI_DIR,
        capture_output=True, text=True
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"tea-host build failed:\n{result.stdout}\n{result.stderr}"
        )
    return [str(binary)]


@pytest.fixture(scope="session")
def transport(request, cfg):
    log_path = request.config.getoption("--session-log")
    selected = request.config.getoption("--transport")
    if selected == "serial":
        t = SerialTransport(
            cfg.port, cfg.baud, tx_eol=cfg.tx_eol, inter_write_delay=cfg.command_pacing,
            conversation_log=log_path,
        )
        t.open()
        t.drain(quiet_for=0.3)  # banner, prompt, boot chatter
        # connect_timeout was declared in config.py but never actually
        # used anywhere - this is that use. See wait_until_responsive's
        # own docstring for why it's needed at all.
        wait_until_responsive(t, cfg.stack_cmd, cfg.connect_timeout)
    elif selected in ("tea-host", "tea-host-double"):
        # Unlike fake_target.py (a line-reading Python script, indifferent
        # to what the line ending actually is), this binary runs the real
        # cli.c: parse_keyq() only commits a line on CRETURN ('\r') and
        # silently treats a bare LFEED as a no-op, exactly like real
        # hardware over a serial port. HostTransport's own default tx_eol
        # ("\n") means no command would ever be seen as complete.
        t = HostTransport(_tea_host_binary(selected == "tea-host-double"),
                          tx_eol=cfg.tx_eol, conversation_log=log_path)
        t.open()
        t.drain(quiet_for=0.3)
    else:
        t = HostTransport(cfg.host_binary, conversation_log=log_path)
        t.open()
        t.drain(quiet_for=0.3)
    yield t
    t.close()


@pytest.fixture(scope="session")
def session(transport, cfg) -> Session:
    return Session(transport, build_codec(cfg), cfg)


@pytest.fixture(scope="session")
def cli(session, cfg) -> Cli:
    """Known state before anything else touches the board.

    Without this, the very first thing a session does - commands()'s bare
    help fetch - runs against whatever the board happens to be holding:
    macros left behind by a person's own terminal session, an earlier
    aborted run's stack contents, whatever. Confirmed live: a "msg" macro
    from a manual test of the here-space fix showed up in commands() and
    failed the coverage gate, because nothing had reset the board between
    that session and this one.
    """
    c = Cli(session, cfg)
    c.reset()
    return c


@pytest.fixture(scope="session")
def commands(cli):
    """Command registry discovered from the target itself."""
    return cli.commands()


@pytest.fixture(scope="session")
def timing(cli, cfg) -> TimingInstrument:
    return TimingInstrument(cli, cfg)


@pytest.fixture(autouse=True)
def _conversation_log_marker(request, transport):
    """Divide the conversation log by test name, if one is active.

    Not load-bearing for anything - just makes a transcript navigable when
    reading it back to understand what a specific test actually said to
    the target, rather than one undifferentiated stream of lines.
    """
    transport.log_marker(request.node.name)
    yield


@pytest.fixture(autouse=True)
def isolation(request, cli):
    """Full reset before each test; snapshot check afterwards.

    Setup uses resetcli rather than a stack clear because clearing the
    parameter stack leaves macros defined, naps pending, and the input
    parser wherever the last test abandoned it. Any of those makes the next
    test depend on execution order.

    Teardown READS the stack before resetting it. Resetting first would
    destroy the evidence the guard exists to report.
    """
    if request.config.getoption("--no-stack-guard"):
        yield
        return
    cli.reset()
    yield
    try:
        cli.assert_baseline(context=request.node.name)
    finally:
        cli.reset()


@pytest.fixture(scope="session", autouse=True)
def timing_snapshot(request, cfg):
    """0stats once before the run, mstats once after - not per test.

    mstats/0stats measure the C-level TEA action stats (the state
    machines: motor updates, telemetry, the CLI's own service action), a
    session-wide concern that exists independently of which CLI command a
    given test happens to be exercising. Resetting and re-snapshotting it
    around every individual test asked the same question a hundred times
    instead of once, and cost a full mstats round-trip (a long, multi-line
    report) on every single test regardless of transport speed - that
    overhead, not link flakiness, was the dominant cause of the slow,
    unreliable runs earlier in this session.

    No assertion: mstats reports only observed max time, never a
    configured limit (see config.timing_report_cmd), so there's nothing to
    compare against yet. This just captures the table so a human can look
    for anything unusual accumulated over the whole session.

    Also snapshots `te` at the same point - empty is the expected/healthy
    state once everything's settled at the end of a run, so a non-empty
    list is itself worth a look. Any bare hex address either report
    printed (an action with no namedAction() name) is resolved
    automatically via Scripts/resolve_action.py, so the summary names a
    function and source line instead of a pointer - see
    pytest_terminal_summary below for where all of this is printed.
    """
    if (
        request.config.getoption("--no-timing-guard")
        or request.config.getoption("--transport") != "serial"
    ):
        yield
        return
    cli = request.getfixturevalue("cli")
    cli.execute(cfg.timing_reset_cmd)
    yield
    mstats_resp = cli.execute(cfg.timing_report_cmd, timeout=max(cfg.command_timeout, 5.0))
    te_resp = cli.execute("te", timeout=max(cfg.command_timeout, 5.0))
    request.config._timing_snapshot_report = mstats_resp.text
    request.config._te_report = te_resp.text
    request.config._resolved_actions = _resolve_unnamed_actions(
        mstats_resp.text, te_resp.text, cfg
    )


def _here_hex(cli, cfg) -> int | None:
    resp = cli.execute("here .h", timeout=max(cfg.command_timeout, 5.0))
    if not resp.ok or not resp.lines:
        return None
    try:
        return int(resp.lines[0].strip(), 16)
    except ValueError:
        return None


@pytest.fixture(scope="session", autouse=True)
def here_space_snapshot(request, cfg):
    """here (the CLI's dictionary/"here" pointer) at session start vs end.

    Every quoted string allot()s space there, and it's only ever reclaimed
    by a full resetcli - not by returning to the prompt, not by anything
    short of that. config.mark_template's `here - allot` tail makes the
    framing itself dictionary-neutral (see tests/test_strings.py), but
    individual tests still legitimately push a handful of their own
    quoted strings in the course of testing quote/strlen/type directly, so
    ending the session at a slightly different "here" than it started
    isn't automatically a bug the way a session-wide drift proportional to
    the number of commands sent would be.

    That's why this reports rather than asserts, unlike stack_health_snapshot:
    there's no clean threshold between "a few tests' own strings" and "a
    real leak" to hang a pass/fail on, but a human glancing at the two
    numbers can tell the difference immediately. This is exactly the
    monitor that would have caught the marker-template leak on its own,
    without needing to reproduce a board hang first.
    """
    if request.config.getoption("--transport") != "serial":
        yield
        return
    cli = request.getfixturevalue("cli")
    start = _here_hex(cli, cfg)
    yield
    end = _here_hex(cli, cfg)
    if start is not None and end is not None:
        request.config._here_space_report = (
            f"here: {start:08X} at session start -> {end:08X} at session end "
            f"({end - start:+d} bytes)"
        )


@pytest.fixture(scope="session", autouse=True)
def stack_health_snapshot(request, cfg):
    """show-sys once at session end: the canary's verdict on stack health.

    Unlike timing_snapshot, this DOES assert. board_cli.c's show_sys() ends
    with a "stack:" line rendered by Robot/diagnostics/canary/canary.c's
    stack_render(): "OK  hwm=<high-water mark>/<limit> B", or WARNING (hwm
    exceeds _Min_Stack_Size), OVERFLOW (usage ran into BSS - the line
    includes a hex dump of the incursion), or CRITICAL (consumed all free
    RAM). Any of those three means the canary caught real corruption
    sometime during the run - not necessarily caused by whichever test
    happens to run last - so this is one check at the end of the whole
    session rather than a per-test assertion.
    """
    if request.config.getoption("--transport") != "serial":
        yield
        return
    cli = request.getfixturevalue("cli")
    yield
    resp = cli.execute(cfg.show_sys_cmd, timeout=max(cfg.command_timeout, 5.0))
    request.config._show_sys_report = resp.text
    stack_line = next((l for l in resp.lines if l.strip().startswith("stack:")), None)
    if stack_line is None:
        return  # show-sys changed shape or didn't answer; reported, not asserted
    status = stack_line.split(None, 2)[1:2]
    assert status and status[0] == "OK", (
        f"canary reports stack health is not OK at session end: {stack_line.strip()!r}\n"
        "WARNING means the high-water mark exceeded _Min_Stack_Size; OVERFLOW "
        "means usage ran into BSS; CRITICAL means it consumed all free RAM. "
        "See Robot/diagnostics/canary/canary.c stack_render()."
    )


# Matches the bare hex address mstats/te print for an action with no
# namedAction() name (tea.c machineStats(): "(machine + 2) & ~3";
# printActionName()/action_key(): "(action + 1) & ~3" - equivalent for
# any real function pointer, since Thumb code addresses only ever land on
# a %4 of 0 or 1, where both formulas round to the same value). Anchored
# to 4-8 hex digits with no letters required, matching the zero-padded
# uppercase form these two words actually print, e.g. "0000BA40".
_HEX_ACTION = re.compile(r"^[0-9A-Fa-f]{4,8}$")


def _mstats_hex_names(report: str) -> list[str]:
    return [t.name for t in parse_timing_table(report) if _HEX_ACTION.match(t.name)]


def _te_hex_names(report: str) -> list[str]:
    """te's rows look nothing like mstats's - "in "/"after ", a formatted
    delay, tabTo(17), then the name - so this is parsed on its own rather
    than reusing parse_timing_table. Deliberately anchored to lines that
    start with the words print_te() actually uses, rather than scanning
    the whole report for hex-looking tokens: a timing VALUE elsewhere in
    the text (e.g. "3250") is valid hex too, and would false-match a blind
    scan.
    """
    names = []
    for line in report.splitlines():
        line = line.strip()
        if line.startswith("in ") or line.startswith("after "):
            tokens = line.split()
            if tokens and _HEX_ACTION.match(tokens[-1]):
                names.append(tokens[-1])
    return names


def _resolve_unnamed_actions(mstats_report: str, te_report: str, cfg) -> str | None:
    """Run resolve_action.py on every bare hex address mstats/te printed.

    namedAction() is opt-in (see resolve_action.py's own docstring for
    why), so most actions never get a name - this turns "here's a hex
    address" into an actual function-and-line recommendation, automatically,
    rather than relying on a human to notice, remember the tool exists, and
    copy the address over by hand.
    """
    script = cfg.resolve_action_script
    if not script or not script.exists():
        return None
    addresses = sorted(set(_mstats_hex_names(mstats_report) + _te_hex_names(te_report)))
    if not addresses:
        return None
    result = subprocess.run(
        [sys.executable, str(script), *addresses],
        capture_output=True, text=True, timeout=15,
    )
    return result.stdout or result.stderr


def pytest_terminal_summary(terminalreporter, exitstatus, config):
    report = getattr(config, "_timing_snapshot_report", None)
    if report:
        terminalreporter.section("mstats over the full session (0stats at start, mstats at end)")
        terminalreporter.write_line(report)

    te_report = getattr(config, "_te_report", None)
    if te_report and te_report.strip():
        terminalreporter.section("te at the end of the session (should be empty if everything settled)")
        terminalreporter.write_line(te_report)

    resolved = getattr(config, "_resolved_actions", None)
    if resolved:
        terminalreporter.section("unnamed actions resolved (Scripts/resolve_action.py)")
        terminalreporter.write_line(resolved)

    show_sys_report = getattr(config, "_show_sys_report", None)
    if show_sys_report:
        terminalreporter.section("show-sys at the end of the session (canary stack check)")
        terminalreporter.write_line(show_sys_report)

    here_space_report = getattr(config, "_here_space_report", None)
    if here_space_report:
        terminalreporter.section("here space: session start vs end (dictionary-leak monitor)")
        terminalreporter.write_line(here_space_report)

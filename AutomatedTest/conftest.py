"""
Fixtures and the two automatic guards.

The guards are the point of this file. Both failure modes they catch are
invisible at the point where they are caused and only show up later
somewhere unrelated, which is exactly the kind of bug a human-written test
suite never covers:

  stack_guard   a command that leaves a value on the parameter stack
  timing_guard  a command that blocks longer than an action's design limit

Both run around every test, with no per-test authoring cost.

Usage:
    pytest --transport=host            # fast, no board
    pytest --transport=serial          # real hardware
    pytest --transport=serial -m hw    # hardware-only tests
"""

from __future__ import annotations

import time

import pytest

from timbre import (
    Cli,
    HostTransport,
    SerialTransport,
    Session,
    TargetConfig,
    TimingInstrument,
    build_codec,
)


def pytest_addoption(parser):
    parser.addoption("--transport", default="host", choices=["host", "serial"])
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
    if config.getoption("--transport") != "host":
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


@pytest.fixture(scope="session")
def transport(request, cfg):
    log_path = request.config.getoption("--session-log")
    if request.config.getoption("--transport") == "serial":
        t = SerialTransport(
            cfg.port, cfg.baud, tx_eol=cfg.tx_eol, inter_write_delay=cfg.command_pacing,
            conversation_log=log_path,
        )
        t.open()
        t.drain(quiet_for=0.3)  # banner, prompt, boot chatter
        _wait_until_responsive(t, cfg)
    else:
        t = HostTransport(cfg.host_binary, conversation_log=log_path)
        t.open()
        t.drain(quiet_for=0.3)
    yield t
    t.close()


def _wait_until_responsive(t, cfg) -> None:
    """Poll with a real command until the target actually answers.

    Real hardware has shown an extended unresponsive window right after the
    port opens - tens of seconds, observed up to roughly a minute - before
    the CLI starts answering at all. Diving straight into framed test
    traffic during that window means every single command burns a full
    command_timeout before failing, which is what made the first ~40
    commands of a run look like a link/corruption problem when the board
    was simply still not listening yet. connect_timeout was declared in
    config.py but never actually used anywhere - this is that use.
    """
    deadline = time.monotonic() + cfg.connect_timeout
    while time.monotonic() < deadline:
        t.write_line(cfg.stack_cmd)
        if t.drain(quiet_for=0.3, limit=1.0):
            return
    raise RuntimeError(
        f"{cfg.port} never answered {cfg.stack_cmd!r} within "
        f"connect_timeout={cfg.connect_timeout}s. Board may still be "
        f"booting, wedged, or something else is holding the port."
    )


@pytest.fixture(scope="session")
def session(transport, cfg) -> Session:
    return Session(transport, build_codec(cfg), cfg)


@pytest.fixture(scope="session")
def cli(session, cfg) -> Cli:
    return Cli(session, cfg)


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


@pytest.fixture(autouse=True)
def timing_guard(request, cfg):
    """No action may exceed its design limit during any test.

    Hardware only. Timings from the host build measure the host.
    """
    if (
        request.config.getoption("--no-timing-guard")
        or request.config.getoption("--transport") != "serial"
    ):
        yield
        return
    instrument = TimingInstrument(request.getfixturevalue("cli"), cfg)
    instrument.reset()
    yield
    instrument.assert_within_limits(context=request.node.name)

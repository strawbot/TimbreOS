#!/usr/bin/env python3
"""
Multi-phase mstats checkpoint run.

A background state machine (e.g. the limb controller, started with
"limbon") and the CLI test suite both run on the same target. A plain
before/after mstats diff around the whole thing can't tell you whether a
change in the numbers came from the machine's own operation, from testing
perturbing it, or from it perturbing testing. Three checkpoints instead of
two can:

    1. baseline   0stats, nothing engaged yet
    2. engaged    send --engage (default: limbon), wait --settle seconds,
                  then mstats - the machine's footprint running alone
    3. tested     run the pytest suite (--transport=serial) while the
                  machine keeps running, then mstats - what testing added
                  on top of an already-engaged machine

Each stage's mstats table is diffed against the one before it, so growth
is attributed to a specific phase rather than the whole run.

--disengage (default: limboff) always runs if --engage succeeded, in a
finally block that covers a pytest crash, a dropped connection, or
anything else going wrong in between - a background state machine left
running on real hardware because a script died partway through is worse
than the numbers it was there to collect. If disengage itself fails, this
says so loudly rather than exiting quietly with the machine still active.

    python3 tools/checkpoint_run.py
    python3 tools/checkpoint_run.py --engage limbon --settle 10
    python3 tools/checkpoint_run.py --engage limbon --settle 10 --pytest-args "-k test_link"
    python3 tools/checkpoint_run.py --no-engage    # baseline vs. tested only, nothing started

Exit code is the pytest subprocess's, so this can gate CI the same way a
plain "pytest --transport=serial" would.
"""

from __future__ import annotations

import argparse
import pathlib
import shlex
import subprocess
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))

from timbre import (  # noqa: E402
    Cli,
    SerialTransport,
    Session,
    TargetConfig,
    build_codec,
    wait_until_responsive,
)
from timbre.timing import ActionTiming, parse_timing_table  # noqa: E402

HARNESS_ROOT = pathlib.Path(__file__).resolve().parent.parent


def say(label: str, text: str) -> None:
    # flush=True: without it, these lines can sit in Python's stdout buffer
    # while the pytest subprocess's own output (written to the same fd
    # directly) lands first when output is redirected to a file or pipe,
    # making the phases print out of chronological order.
    print(f"[{label:^9}] {text}", flush=True)


def open_cli(cfg: TargetConfig) -> tuple[SerialTransport, Cli]:
    transport = SerialTransport(
        cfg.port, cfg.baud, tx_eol=cfg.tx_eol, inter_write_delay=cfg.command_pacing
    )
    transport.open()
    transport.drain(quiet_for=0.3)
    wait_until_responsive(transport, cfg.stack_cmd, cfg.connect_timeout)
    cli = Cli(Session(transport, build_codec(cfg), cfg), cfg)
    return transport, cli


def snapshot(cli: Cli, cfg: TargetConfig, *, reset: bool) -> list[ActionTiming]:
    if reset:
        cli.execute(cfg.timing_reset_cmd)
        return []
    resp = cli.execute(cfg.timing_report_cmd, timeout=max(cfg.command_timeout, 5.0))
    if not resp.text.strip():
        # Observed once straight after reopening the connection (e.g. right
        # after pytest closes its own): a blank response even though the
        # target's actual state clearly wasn't empty on direct recheck. A
        # brief settle and one retry has cleared it every time since.
        time.sleep(1.0)
        resp = cli.execute(cfg.timing_report_cmd, timeout=max(cfg.command_timeout, 5.0))
    return parse_timing_table(resp.text)


def diff(before: list[ActionTiming], after: list[ActionTiming]) -> list[str]:
    b = {t.name: t.longest for t in before}
    a = {t.name: t.longest for t in after}
    lines = []
    for name in sorted(set(b) | set(a)):
        bv, av = b.get(name), a.get(name)
        if bv is None:
            lines.append(f"  + {name:<20} new: {av} us")
        elif av is None:
            lines.append(f"  - {name:<20} gone (was {bv} us)")
        elif av != bv:
            lines.append(f"  ~ {name:<20} {bv:>7} -> {av:<7} us  (delta {av - bv:+d})")
    return lines or ["  (no change)"]


def print_diff(title: str, before: list[ActionTiming], after: list[ActionTiming]) -> None:
    print(f"\n=== {title} ===")
    for line in diff(before, after):
        print(line)


def main() -> None:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--port")
    ap.add_argument("--engage", default="limbon",
                     help="command that starts the background machine (default: limbon)")
    ap.add_argument("--disengage", default="limboff",
                     help="command that stops it again, always run if --engage succeeded "
                          "(default: limboff)")
    ap.add_argument("--no-engage", action="store_true",
                     help="skip the engage/disengage phases entirely - baseline vs. tested only")
    ap.add_argument("--settle", type=float, default=10.0,
                     help="seconds to let the engaged machine run before its own mstats (default: 10)")
    ap.add_argument("--pytest-args", default="",
                     help="extra args for the pytest invocation, e.g. '-k test_link'")
    args = ap.parse_args()

    cfg = TargetConfig()
    if args.port:
        cfg.port = args.port

    transport, cli = open_cli(cfg)
    port_open = True
    say("baseline", "0stats")
    snapshot(cli, cfg, reset=True)
    baseline = snapshot(cli, cfg, reset=False)

    engaged = baseline
    tested = baseline
    settled = None
    settled_te = None
    result = None
    engaged_ok = False

    try:
        if not args.no_engage:
            say("engage", f"sending {args.engage!r}")
            resp = cli.execute(args.engage)
            if resp.errors:
                print(
                    f"warning: {args.engage!r} reported an error: {resp.errors} - "
                    f"is it actually flashed on this build? Continuing without it engaged.",
                    file=sys.stderr,
                )
            else:
                engaged_ok = True
            say("settle", f"{args.settle}s")
            time.sleep(args.settle)
            say("engaged", "mstats")
            engaged = snapshot(cli, cfg, reset=False)

        # Release the port before pytest opens its own connection - two
        # processes holding it at once doesn't fail loudly, it just looks
        # like a wedged target (see timbre.transport.check_port_not_already_open).
        transport.close()
        port_open = False

        say("tested", "running pytest --transport=serial"
            + (" (machine still engaged)" if engaged_ok else ""))
        pytest_cmd = [
            sys.executable, "-m", "pytest", "--transport=serial",
            # This script's own snapshots are what's being compared here;
            # pytest's own timing_snapshot fixture would 0stats at its
            # start and wipe out the engage phase's accumulated numbers
            # before the "tested" mstats below ever saw them.
            "--no-timing-guard",
        ]
        if args.pytest_args:
            pytest_cmd += shlex.split(args.pytest_args)
        result = subprocess.run(pytest_cmd, cwd=str(HARNESS_ROOT))

        transport, cli = open_cli(cfg)
        port_open = True
        say("final", "mstats")
        tested = snapshot(cli, cfg, reset=False)
    finally:
        # Runs even if pytest crashed or the connection dropped mid-run:
        # a state machine left running on real hardware because this
        # script died partway through is worse than the numbers it was
        # collecting. A failure here is reported loudly, not swallowed.
        if engaged_ok:
            try:
                if not port_open:
                    transport, cli = open_cli(cfg)
                    port_open = True
                say("disengage", f"sending {args.disengage!r}")
                resp = cli.execute(args.disengage)
                if resp.errors:
                    print(
                        f"WARNING: {args.disengage!r} reported an error: {resp.errors} - "
                        f"the machine may still be running. Check it directly.",
                        file=sys.stderr,
                    )
                else:
                    # Does it actually return to the idle state, or does
                    # something linger (a scheduled action still in te, an
                    # inflated max time that never resets)? 0stats then a
                    # real idle window, not an immediate mstats, since a
                    # lingering action might only show up once it's had a
                    # chance to run again.
                    say("settle", "1s before 0stats")
                    time.sleep(1.0)
                    snapshot(cli, cfg, reset=True)
                    say("settle", "10s idle")
                    time.sleep(10.0)
                    say("settled", "mstats + te")
                    settled = snapshot(cli, cfg, reset=False)
                    settled_te = cli.execute(
                        "te", timeout=max(cfg.command_timeout, 5.0)
                    ).text
            except Exception as exc:
                print(
                    f"WARNING: could not send {args.disengage!r} to disengage - "
                    f"the machine may still be running: {exc}",
                    file=sys.stderr,
                )
        if port_open:
            transport.close()

    if not args.no_engage:
        print_diff(f"baseline -> engaged (footprint of {args.engage!r} alone)", baseline, engaged)
    print_diff("engaged -> tested (effect of running the suite while engaged)", engaged, tested)
    print_diff("baseline -> tested (overall)", baseline, tested)

    if settled is not None:
        print(f"\n=== idle 10s after {args.disengage!r} - eyeball this for anything left "
              f"over from {args.engage!r} ===")
        if settled:
            for t in sorted(settled, key=lambda t: -t.longest):
                print(f"  {t.longest:>7} us  {t.name}")
        else:
            print("  (nothing recorded)")
        print(f"\n=== te at the same point - should show only the perpetual "
              f"heartbeats, nothing from {args.engage!r} ===")
        print(settled_te.strip() if settled_te and settled_te.strip() else "  (empty)")

    raise SystemExit(result.returncode if result is not None else 1)


if __name__ == "__main__":
    main()

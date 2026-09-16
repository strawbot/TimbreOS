#!/usr/bin/env python3
"""
First contact with a real TimbreOS board.

Run this before the test suite. It walks the protocol layers in order and
stops at the first one that fails, so a bad run tells you *which* layer is
wrong instead of producing a page of pytest failures that all trace back to
one config line.

It reads the target and reports the config values it infers. It does not
write them: guessing silently is how a harness ends up confidently testing
something other than what you think.

    python tools/check_link.py
    python tools/check_link.py --port /dev/cu.usbserial-FTVGKHLB3
    python tools/check_link.py --host          # against the fake target
"""

from __future__ import annotations

import argparse
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))

from timbre import (  # noqa: E402
    Cli,
    HostTransport,
    SerialTransport,
    Session,
    TargetConfig,
    build_codec,
)
from timbre.stack import _ints  # noqa: E402

OK, BAD, INFO = "  ok  ", " FAIL ", "  ..  "
findings: list[str] = []


def say(tag: str, text: str) -> None:
    print(f"[{tag}] {text}")


def infer(setting: str, value: str) -> None:
    findings.append(f"    {setting} = {value!r}")
    say(OK, f"infers {setting} = {value!r}")


def fail(text: str, hint: str = "") -> None:
    say(BAD, text)
    if hint:
        print(f"\n       {hint}\n")
    summary()
    sys.exit(1)


def summary() -> None:
    if findings:
        print("\n  Config values inferred from the target:\n")
        print("\n".join(findings))
        print("\n  Set these in timbre/config.py, then run the suite.\n")


# ----------------------------------------------------------------------
def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port")
    ap.add_argument("--baud", type=int)
    ap.add_argument("--host", action="store_true", help="use the fake target")
    args = ap.parse_args()

    cfg = TargetConfig()
    if args.port:
        cfg.port = args.port
    if args.baud:
        cfg.baud = args.baud

    # -- 1. the port opens ------------------------------------------------
    if args.host:
        transport = HostTransport(cfg.host_binary)
        say(INFO, "using the host build of the CLI")
    else:
        transport = SerialTransport(
            cfg.port, cfg.baud, tx_eol=cfg.tx_eol, inter_write_delay=cfg.command_pacing
        )
        say(INFO, f"opening {cfg.port} at {cfg.baud}")
    transport.open()
    say(OK, "port open")

    banner = transport.drain(quiet_for=0.5, limit=3.0)
    if banner:
        say(OK, f"target sent {len(banner)} line(s) on connect")
        for line in banner[:4]:
            print(f"       | {line}")
    else:
        say(INFO, "nothing on connect (normal if the board is already running)")

    session = Session(transport, build_codec(cfg), cfg)
    cli = Cli(session, cfg)

    # -- 2. the target answers at all -------------------------------------
    lines = session.raw(cfg.tx_eol, quiet_for=0.5)
    lines += session.raw(cfg.stack_cmd + cfg.tx_eol, quiet_for=0.5)
    if not lines:
        fail(
            f"no response to {cfg.stack_cmd!r}",
            "Check baud rate, TX/RX orientation, and that the board is running.\n"
            "       A wrong baud usually gives garbage rather than silence, so\n"
            "       silence points at wiring or a target that is not booted.",
        )
    say(OK, f"target responded to {cfg.stack_cmd!r}")
    for line in lines:
        print(f"       | {line}")

    # -- 3. prompt --------------------------------------------------------
    for line in lines:
        if line and not line.strip().startswith(cfg.prompt.strip()):
            continue
        if line.startswith(cfg.prompt):
            say(OK, f"prompt {cfg.prompt!r} confirmed")
            break
    else:
        say(INFO, f"prompt {cfg.prompt!r} not seen; check config.prompt")

    # -- 4. stack format --------------------------------------------------
    stack_text = "\n".join(lines)
    if "stack items" in stack_text:
        infer("stack_format", "count_first")
    elif "<" in stack_text and ">" in stack_text:
        infer("stack_format", "bracket")
    else:
        say(INFO, "could not recognise the .s format; check config.stack_format")

    # -- 5. marker framing (requirement R1) -------------------------------
    resp = cli.execute(cfg.stack_cmd)
    if resp.timed_out:
        fail(
            "framed command timed out",
            f"The marker template {cfg.mark_template!r} must print its text ALONE on a\n"
            f"       line. If it prints inline, the harness cannot tell a marker\n"
            f"       from the echo of the command that contains it.\n"
            f"       Sent: {resp.sent}",
        )
    if resp.frame_lost:
        fail("frame lost: the command line was discarded before it ran",
             "Usually config.max_line_length is too high for the real buffer.")
    say(OK, "marker framing works")

    # -- 6. error detection -----------------------------------------------
    bad = session.execute("definitely_not_a_command_xyzzy")
    if not bad.lines:
        say(INFO, "target said nothing about an unknown word")
    else:
        print(f"       | {bad.lines[0]}")
        if bad.errors:
            say(OK, f"error_pattern matches: {cfg.error_pattern!r}")
        else:
            fail(
                "error_pattern does not match the target's error output",
                "This is the most consequential line in config.py. Until it\n"
                "       matches, every negative test in the suite passes while the\n"
                "       CLI rejects everything, and the run reports green on a\n"
                "       completely broken interpreter.",
            )

    # -- 7. stack display order -------------------------------------------
    cli.reset()
    cli.execute("11 22 33 drop")
    raw = cli.execute(cfg.stack_cmd)
    nums = [v for v in _ints(raw.text.replace(":", " ")) if v in (11, 22, 33)]
    if nums == [11, 22]:
        infer("stack_display_order", "bottom_first")
    elif nums == [22, 11]:
        infer("stack_display_order", "top_first")
    else:
        say(INFO, f"could not settle display order; `.s` gave {raw.text!r}")

    # -- 8. introspection --------------------------------------------------
    cli.reset()
    try:
        names = cli.word_names()
        say(OK, f"{len(names)} commands discovered")
        print(f"       | {' '.join(names[:12])}{' ...' if len(names) > 12 else ''}")
    except Exception as exc:
        fail(f"could not read the command list: {exc}",
             "Check config.words_cmd.")

    sample = next((n for n in names if n not in (cfg.reset_cmd, cfg.clear_stack_cmd)), None)
    if sample:
        spec = cli.describe(sample)
        print(f"       | {sample}: {spec.raw.strip()}")
        if spec.args or spec.results or "--" in spec.raw:
            say(OK, "stack comments are parseable")
        else:
            say(INFO, "no parseable stack effect; the conformance sweep will skip")

    # -- 9. timing instrument ----------------------------------------------
    times = cli.execute(cfg.timing_report_cmd, timeout=5.0)
    if times.ok and times.lines:
        say(OK, f"timing instrument responded with {len(times.lines)} line(s)")
        for line in times.lines[:4]:
            print(f"       | {line}")
    else:
        say(INFO, f"no timing table from {cfg.timing_report_cmd!r}; "
                  "the timing guard will need config.timing_report_cmd set")

    cli.reset()
    transport.close()
    print()
    say(OK, "all layers responded")
    summary()


if __name__ == "__main__":
    main()

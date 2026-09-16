"""
Layer 2: tests generated from the target's own command table.

Two things here.

The coverage gate answers the question you actually care about — is there a
full test set for the CLI — mechanically rather than by belief. The command
list comes from the target, so adding a word to the firmware fails the build
until someone writes a case for it. Without this, a "complete" test set
decays quietly with every release.

The non-blocking sweep turns the TimbreOS design promise into an executable
assertion. Every command is run while the execution-time instrument is armed.
Any command that stalls an action past its limit names itself.
"""

import json
import pathlib

import pytest

COVERAGE_FILE = pathlib.Path(__file__).parent / "command_coverage.json"

# Commands the sweep must not invoke blindly: they reset, erase, move
# actuators, or otherwise change the world.
#
# TODO: fill this in. Everything listed here still needs a hand-written test;
# the coverage gate enforces that.
# Words whose purpose IS to alter the whole stack. Sentinel floors cannot
# survive them by definition, so they are excluded from floor checks rather
# than reported as offenders.
#
# TimbreOS exposes dup, drop, swap and over. There is no rot.
#
# These need no hand-written tests: their stack comments fully determine
# their behaviour, so test_stack_effects.py derives the assertions from the
# comments. Add a new shuffler with a comment and it is covered.
#
# They are listed here only to exempt them from sentinel FLOOR checks, since
# rearranging the stack is the entire point of them.
STACK_WORDS = {
    "sp!",
    ".s",
    "drop",
    "dup",
    "swap",
    "over",
}

DESTRUCTIVE = {
    "reset",
    "reboot",
    "erase",
    "flash",
    "poke",
    # Drops the board out of the CLI into a bootloader; the session (and
    # anything talking to this port) doesn't come back without a reflash.
    "dfu",
    # Reprograms real I2C addressing on the limb boards' MCP4728s. Its own
    # help text calls it "one-time" - not something a sweep should invoke
    # incidentally.
    "limbio-assign",
    # Zero-arity DAC actuation: ramps a real DAC output with no argument.
    "up",
    "down",
}


def _tested_commands() -> set[str]:
    if not COVERAGE_FILE.exists():
        return set()
    data = json.loads(COVERAGE_FILE.read_text())
    return set(data.get("covered", []))


# ----------------------------------------------------------------------
# Discovery sanity
# ----------------------------------------------------------------------
def test_command_table_is_discoverable(commands):
    assert commands, "target returned no command names"
    assert len(commands) > 3, f"suspiciously short command list: {list(commands)}"


def test_every_command_has_help(commands):
    missing = sorted(n for n, s in commands.items() if not s.help.strip())
    assert not missing, f"commands with no help text: {missing}"


def test_every_command_declares_a_stack_effect(commands):
    """Undeclared arity means the harness cannot generate negative tests."""
    missing = sorted(n for n, s in commands.items() if not s.raw or "--" not in s.raw)
    if missing:
        pytest.xfail(f"no parseable stack effect for: {missing}")


# ----------------------------------------------------------------------
# The coverage gate
# ----------------------------------------------------------------------
def test_all_commands_are_covered(commands):
    """Fail the build when a command exists with no test behind it."""
    uncovered = sorted(set(commands) - _tested_commands())
    assert not uncovered, (
        f"{len(uncovered)} CLI command(s) have no test:\n  "
        + "\n  ".join(uncovered)
        + f"\n\nAdd cases, then list them in {COVERAGE_FILE.name}."
    )


# ----------------------------------------------------------------------
# The non-blocking property
# ----------------------------------------------------------------------
@pytest.mark.hw
def test_no_command_blocks_an_action(cli, commands, timing, cfg):
    """The core TimbreOS invariant, swept across the whole command set.

    Run on hardware only. Failures name the action and the command that was
    executing when it overran, which is usually enough to find the busy-wait
    without a debugger.
    """
    offenders = []
    for name, spec in sorted(commands.items()):
        if name in DESTRUCTIVE or spec.arity > 0 or spec.takes_text:
            continue
        timing.reset()
        cli.execute(name)
        cli.clear_stack()
        for violation in timing.violations():
            offenders.append((name, violation))

    assert not offenders, "commands that stalled an action past its limit:\n" + "\n".join(
        f"  {cmd!r} -> {v.name} took {v.longest}{cfg.timing_units} "
        f"(limit {v.limit}{cfg.timing_units})"
        for cmd, v in offenders
    )


@pytest.mark.hw
def test_idle_system_meets_all_limits(timing):
    """Baseline: with nothing driving it, every action is inside its budget."""
    timing.reset()
    timing.assert_within_limits(context="idle")


@pytest.mark.hw
def test_report_timing_headroom(timing, cfg, record_property):
    """Not an assertion — records worst-case headroom as build metadata.

    Tracked over time, this is an early warning: an action drifting from 60%
    headroom to 15% across releases is a problem long before it trips a limit.
    """
    for t in timing.worst(10):
        record_property(f"timing.{t.name}", t.longest)
        if t.headroom_pct is not None:
            record_property(f"headroom.{t.name}", round(t.headroom_pct, 1))


# ----------------------------------------------------------------------
# Smoke sweep
# ----------------------------------------------------------------------
def test_every_zero_arity_command_is_safe_to_run(cli, commands, cfg):
    """No command should hang, crash, or corrupt the stack when invoked bare.

    Stack depth is checked after each command rather than left to the
    teardown guard, so a leak is attributed to the command that caused it
    instead of to the test. The guard is a backstop for tests that forget;
    a sweep like this should name its own offender.

    Note the shape: clearing the stack at the END of a test defeats the
    teardown guard by destroying the evidence. Clear between iterations,
    not after the last one.
    """
    failures = []
    for name, spec in sorted(commands.items()):
        if name in DESTRUCTIVE or spec.arity > 0 or spec.takes_text:
            continue
        cli.clear_stack()
        resp = cli.execute(name)

        if resp.timed_out:
            failures.append(f"{name}: no response (interpreter may be blocked)")
            continue
        if resp.frame_lost:
            failures.append(f"{name}: command line discarded before executing")
            continue
        if resp.crc_failures:
            failures.append(f"{name}: {resp.crc_failures} CRC failure(s)")
            continue

        # A declared result may be PRINTED or PUSHED, and `( -- n )` cannot
        # tell you which. So both outcomes are accepted and only a depth
        # outside that range is a failure.
        #
        # FIRMWARE RECOMMENDATION: make the notation distinguish them, e.g.
        # `( -- n )` for pushed and `( -- ) .n` for printed. It is a small
        # convention change that lets this test assert exact depth, which
        # turns a loose check into a precise one across every command.
        depth = cli.depth()
        allowed = {cfg.baseline_depth, cfg.baseline_depth + len(spec.results)}
        if depth not in allowed:
            failures.append(
                f"{name}: stack depth {depth}, declared stack effect allows "
                f"{sorted(allowed)} ({spec.raw.strip() or 'no declared effect'})"
            )

    assert not failures, "commands that misbehaved when run bare:\n  " + "\n  ".join(failures)

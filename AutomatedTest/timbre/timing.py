"""
Layer 2: the on-target longest-execution-time instrument.

This is the most valuable thing the harness can attach to, because TimbreOS
makes a hard architectural promise — actions do not block — and the
instrument already measures exactly the quantity that promise is about.

Wiring it into an autouse fixture means every test in the suite becomes a
latency regression test at no authoring cost. Someone adds a busy-wait to a
command six months from now and the test that fails is whichever one
happened to exercise it, with the offending action named. That is a far
better outcome than finding it on a robot in the field.

Measure on hardware. Host-build timings are meaningless for this.
"""

from __future__ import annotations

import re
from dataclasses import dataclass

from .cli import Cli
from .config import TargetConfig


@dataclass
class ActionTiming:
    name: str
    longest: int
    limit: int | None = None
    runs: int | None = None

    @property
    def exceeded(self) -> bool:
        return self.limit is not None and self.longest > self.limit

    @property
    def headroom_pct(self) -> float | None:
        if not self.limit:
            return None
        return 100.0 * (self.limit - self.longest) / self.limit


class TimingViolation(AssertionError):
    pass


# Real output (tea.c machineStats(), word "mstats") looks like:
#
#      mstats: 3
#          123 us  motor_update
#            5 ms  telemetry_tx
#           47 ns  cli_service
#     latency in us:     min     max     avg
#     ...
#
# i.e. value, unit, name - the reverse order of what was first assumed here,
# and with no declared limit or run count at all: the firmware reports only
# the observed max, never a configured ceiling. So limit/runs stay None
# until real per-action limits are sourced from somewhere else (there is no
# such value on the target to read) and wired in below - until then,
# assert_within_limits() has nothing to compare against and can't fail.
#
# The " mstats: N" header and the trailing "latency in us: ..." block (unit-
# less names like "in(secs(1),)") don't match this row shape and are
# skipped like any other non-data line.
_ROW = re.compile(
    r"^\s*(?P<longest>\d+)\s*(?P<unit>us|ms|ns)\s+(?P<name>\S+)\s*$"
)

_TO_US = {"us": 1, "ms": 1000, "ns": 1}  # ns truncates to 0; finer than we track


def parse_timing_table(text: str) -> list[ActionTiming]:
    rows: list[ActionTiming] = []
    for line in text.splitlines():
        match = _ROW.match(line)
        if not match:
            continue  # header, separator, or footer
        value = int(match.group("longest"))
        us = value * _TO_US[match.group("unit")]
        rows.append(ActionTiming(name=match.group("name"), longest=us))
    return rows


class TimingInstrument:
    def __init__(self, cli: Cli, cfg: TargetConfig):
        self.cli = cli
        self.cfg = cfg

    def reset(self) -> None:
        self.cli.expect_ok(self.cfg.timing_reset_cmd)

    def report(self) -> list[ActionTiming]:
        resp = self.cli.expect_ok(
            self.cfg.timing_report_cmd,
            timeout=max(self.cfg.command_timeout, 5.0),
        )
        return parse_timing_table(resp.text)

    def violations(self) -> list[ActionTiming]:
        return [t for t in self.report() if t.exceeded]

    def assert_within_limits(self, context: str = "") -> None:
        bad = self.violations()
        if not bad:
            return
        units = self.cfg.timing_units
        detail = "\n".join(
            f"  {t.name}: {t.longest}{units} > limit {t.limit}{units}"
            f"{f' over {t.runs} runs' if t.runs else ''}"
            for t in bad
        )
        where = f" during {context}" if context else ""
        raise TimingViolation(
            f"{len(bad)} action(s) exceeded their execution-time limit{where}:\n{detail}"
        )

    def worst(self, n: int = 5) -> list[ActionTiming]:
        return sorted(self.report(), key=lambda t: t.longest, reverse=True)[:n]

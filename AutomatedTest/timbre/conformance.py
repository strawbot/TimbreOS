"""
Verifying stack comments against actual behaviour.

The useful observation: when every result name in a stack comment is drawn
from its argument names, the comment does not merely describe the command,
it *specifies* it completely.

    dup  ( n -- n n )       -> push one probe, expect it twice
    drop ( n -- )           -> push one probe, expect nothing
    swap ( a b -- b a )     -> push two probes, expect them reversed
    over ( a b -- a b a )   -> push two, expect three in that arrangement

So the test is derived from the comment rather than written by hand, and any
stack word added later is covered the moment its comment exists. Comment and
code drift apart constantly; this makes the comment load-bearing.

Where a result name is NOT an argument name (`add ( a b -- sum )`), values
cannot be derived and only the depth delta is checkable. Still worth having:
a command declaring two results while producing one is a real bug, and the
declared arity drives the underflow sweep.

Convention: arguments and results read left to right as bottom to top, so
the rightmost name is on top.
"""

from __future__ import annotations

from dataclasses import dataclass

PROBE_BASE = 0x7E570000


def probe(index: int) -> int:
    return PROBE_BASE | (index & 0xFFFF)


def is_probe(value: int) -> bool:
    return (value & 0xFFFF0000) == PROBE_BASE


def assign_probes(args: list[str]) -> tuple[dict[str, int], list[int]]:
    """Map each argument name to a distinct probe value.

    Returns the name->value map and the push order (bottom first).
    """
    mapping: dict[str, int] = {}
    order: list[int] = []
    for i, name in enumerate(args):
        if name not in mapping:
            mapping[name] = probe(i + 1)
        order.append(mapping[name])
    return mapping, order


def derive_expected(args: list[str], results: list[str]) -> tuple[int, ...] | None:
    """Expected final stack, or None when the comment does not determine it."""
    if not results:
        return ()
    if not all(r in args for r in results):
        return None
    mapping, _ = assign_probes(args)
    return tuple(mapping[r] for r in results)


@dataclass
class EffectResult:
    name: str
    declared: str
    derivable: bool
    expected: tuple[int, ...] | None
    observed: tuple[int, ...]
    depth_delta: int
    declared_delta: int
    printed_output: bool
    ok: bool
    reason: str = ""

    def __str__(self) -> str:
        def fmt(values):
            return "[" + " ".join(f"P{v & 0xFFFF}" if is_probe(v) else str(v)
                                  for v in values) + "]"
        if self.derivable:
            return (
                f"{self.name} {self.declared}\n"
                f"      expected {fmt(self.expected)}, got {fmt(self.observed)}"
            )
        return (
            f"{self.name} {self.declared}\n"
            f"      declared depth change {self.declared_delta:+d}, "
            f"observed {self.depth_delta:+d}"
            f"{' (results were printed, not pushed)' if self.printed_output else ''}"
        )


def check_effect(cli, spec, floor: int = 2, derivable: bool | None = None) -> EffectResult:
    """Run one command with probe arguments and compare against its comment.

    derivable=False forces the depth-only path even when derive_expected()
    would naively call it derivable. Needed for words like sqrt ( f -- f )
    where the result name matches an argument name because both are
    labeled by TYPE (float), not because the command claims to hand the
    same VALUE back unchanged - derive_expected can't tell those two
    conventions apart from the comment text alone. See callers for the
    curated list.
    """
    cfg = cli.cfg
    # A stack clear, not a full reset: this runs inside a test the isolation
    # fixture has already reset, and a reset per command would dominate the
    # runtime of the sweep.
    cli.clear_stack()
    marks = cli.push_sentinels(floor)

    _, push_order = assign_probes(spec.args)
    if push_order:
        cli.execute(" ".join(str(v) for v in push_order))

    before = cli.stack().depth
    resp = cli.execute(spec.name)
    after = cli.stack()

    # Sentinels are scaffolding, not part of the command's working set.
    observed = tuple(v for v in after.values if not is_sentinel_value(v, cfg))
    depth_delta = after.depth - before
    declared_delta = len(spec.results) - len(spec.args)
    expected = derive_expected(spec.args, spec.results) if derivable is not False else None

    result = EffectResult(
        name=spec.name,
        declared=spec.raw.strip() or "(no comment)",
        derivable=expected is not None,
        expected=expected,
        observed=observed,
        depth_delta=depth_delta,
        declared_delta=declared_delta,
        printed_output=bool(resp.lines),
        ok=True,
    )

    if after.sentinels(cfg.sentinel_base) != marks:
        result.ok = False
        result.reason = "reached below its arguments (sentinel floor disturbed)"
        return result

    if resp.errors:
        result.ok = False
        result.reason = f"errored on valid arguments: {resp.errors}"
        return result

    if expected is not None:
        if observed != expected:
            result.ok = False
            result.reason = "stack contents do not match the declared effect"
        return result

    # Not derivable. A declared result may be printed rather than pushed, so
    # accept either, but not a third answer.
    allowed = {declared_delta, -len(spec.args)}
    if depth_delta not in allowed:
        result.ok = False
        result.reason = (
            f"depth changed by {depth_delta:+d}; the comment allows "
            f"{sorted(allowed)} (pushed or printed results)"
        )
    return result


def is_sentinel_value(value: int, cfg) -> bool:
    return (value & 0xFFFF0000) == (cfg.sentinel_base & 0xFFFF0000)

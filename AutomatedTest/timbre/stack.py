"""
Parameter stack snapshots and sentinel scaffolding.

`.s` reports depth and every value, which is a much stronger instrument than
a bare depth count. Two things become possible:

  1. A snapshot can be compared exactly, not just by length. "The stack is
     three deep" and "the stack still holds the three values I put there"
     are different claims, and only the second one is worth much.

  2. Sentinels. Push improbable marked values beneath the real parameters,
     then check they survive. This is what distinguishes a command that
     checks depth BEFORE popping from one that pops first and reports the
     error afterwards. Both report the error correctly. Only the second one
     leaves the stack corrupted, and with an empty stack the two are
     indistinguishable.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field


class StackParseError(RuntimeError):
    pass


@dataclass(frozen=True)
class StackSnapshot:
    """Depth and contents at a moment in time. Index 0 is the bottom."""

    depth: int
    values: tuple[int | float, ...] = field(default=())

    def __post_init__(self):
        if self.values and len(self.values) != self.depth:
            raise StackParseError(
                f"stack reports depth {self.depth} but lists "
                f"{len(self.values)} values: {self.values}. "
                "config.stack_format probably does not match the target."
            )

    @property
    def top(self) -> int:
        if not self.values:
            raise StackParseError("no values in snapshot")
        return self.values[-1]

    def sentinels(self, base: int) -> tuple[int, ...]:
        return tuple(v for v in self.values if is_sentinel(v, base))

    def __str__(self) -> str:
        if not self.values:
            return f"<{self.depth}>"
        return f"<{self.depth}> " + " ".join(
            f"S{v & 0xFFFF}" if is_sentinel(v) else str(v) for v in self.values
        )


def sentinel(index: int, base: int = 0x5EED0000) -> int:
    return base | (index & 0xFFFF)


def is_sentinel(value: int | float, base: int = 0x5EED0000) -> bool:
    return isinstance(value, int) and (value & 0xFFFF0000) == (base & 0xFFFF0000)


# ----------------------------------------------------------------------
_BRACKET = re.compile(r"<\s*(\d+)\s*>")
_INT = re.compile(r"^-?(0[xX][0-9a-fA-F]+|\d+)$")
# A 64 bit float build (-DFLOAT_DOUBLE) can tell floats from ints on the
# stack, so its `.s` prints floats the way `.f` does: "3.1416",
# "1.5000e10", "NAN", "INF".
_FLOAT = re.compile(r"^-?(\d+\.\d*(e-?\d+)?|NAN|INF)$")


def _ints(text: str) -> list[int | float]:
    values = []
    for tok in text.split():
        if _INT.match(tok):
            values.append(int(tok, 0))
        elif _FLOAT.match(tok):
            values.append(float(tok.replace("NAN", "nan").replace("INF", "inf")))
    return values


def parse_stack_output(
    text: str,
    fmt: str = "count_first",
    order: str = "bottom_first",
) -> StackSnapshot:
    """Parse `.s` output.

    Deliberately strict. A parser that quietly returns a plausible-looking
    snapshot from output it did not understand would make every stack
    assertion in the suite meaningless, so a format mismatch raises here
    rather than degrading.
    """
    body = text.strip()

    def finish(depth: int, values: tuple[int | float, ...]) -> StackSnapshot:
        # One invariant for the whole harness: index 0 is the bottom of the
        # stack and the last element is the top. Display order is a target
        # quirk that stops here.
        if order == "top_first":
            values = tuple(reversed(values))
        elif order != "bottom_first":
            raise StackParseError(f"unknown stack_display_order {order!r}")
        return StackSnapshot(depth, values)

    if fmt == "bracket":
        match = _BRACKET.search(body)
        if not match:
            raise StackParseError(
                f"no <depth> field in {text!r}. Set config.stack_format to "
                f"'count_first' or 'values_only' if `.s` uses another shape."
            )
        return finish(int(match.group(1)), tuple(_ints(body[match.end():])))

    if fmt == "count_first":
        nums = _ints(body.replace(":", " "))
        if not nums:
            raise StackParseError(f"no integers in {text!r}")
        return finish(nums[0], tuple(nums[1:]))

    if fmt == "values_only":
        nums = _ints(body)
        return finish(len(nums), tuple(nums))

    raise StackParseError(f"unknown stack_format {fmt!r}")

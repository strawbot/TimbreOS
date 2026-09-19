"""
Layer 2: a typed API over the interpreter.

Encodes the CLI grammar so tests never build command strings by hand:
values are pushed left to right, the command word follows them, and any
trailing text belongs to the command because the command parses the input
stream itself as it runs.

    cli.call("poke", 0x20000000, 0xDEADBEEF)   ->  "536870912 3735928559 poke"
    cli.call("name", text="motor-left")        ->  "name motor-left"

The stack helpers matter more than they look. The parameter stack is shared
mutable state that outlives a command, so a test that leaves a value behind
silently changes the meaning of every test after it. depth() makes that
visible; conftest turns it into an automatic check around every test.
"""

from __future__ import annotations

import contextlib
import re
from dataclasses import dataclass, field

from .config import TargetConfig
from .session import ProtocolError, Response, Session
from .stack import (
    StackParseError,
    StackSnapshot,
    is_sentinel,
    parse_stack_output,
    sentinel,
)


class StackError(RuntimeError):
    pass


@dataclass
class CommandSpec:
    name: str
    args: list[str] = field(default_factory=list)
    results: list[str] = field(default_factory=list)
    help: str = ""
    raw: str = ""
    # True whenever parse_spec() actually matched a stack-effect comment -
    # _STACK_EFFECT or its dash-less fallback _ARGS_ONLY - regardless of
    # whether that comment turned out to declare zero args and zero
    # results (a real, parseable "( - )", as opposed to no comment at
    # all). args/results being empty can't answer that question on their
    # own; this can.
    has_declared_effect: bool = False

    @property
    def arity(self) -> int:
        return len(self.args)

    @property
    def takes_text(self) -> bool:
        """True if the command consumes trailing text from the input stream.

        Text arguments do NOT appear in the stack comment, because they are
        read from the input stream rather than popped. `: ( -- ) <name>`
        declares no stack effect and still consumes the next word. So the
        marker to look for is `<...>` in the help line, not an argument name.

        Getting this wrong makes the conformance sweep invoke text commands
        with no text, which they correctly reject, producing a page of
        failures that are the harness's fault rather than the firmware's.
        """
        if re.search(r"<[^>]+>", self.raw):
            return True
        return any(a.lower() in ("text", "str", "string", "name") for a in self.args)

    @property
    def is_immediate(self) -> bool:
        """True for control-flow/compiler words marked "[i]" in their help.

        `again`, `if`, `;`, `[`, ... only mean something while a macro build
        is in progress (or, for a few, only *because* one is): they jump on
        the return stack, toggle compile state, or terminate a definition
        that isn't there. Their stack comment - often none at all - can't
        capture that. A sweep that invokes every zero-arity command bare to
        check its stack effect must skip these for a different reason: not
        "wrong depth", but "undefined interpreter state outside a macro".
        """
        return bool(re.search(r"\[i\]", self.raw))


class Cli:
    def __init__(self, session: Session, cfg: TargetConfig):
        self.s = session
        self.cfg = cfg
        self._specs: dict[str, CommandSpec] | None = None

    # -- execution --------------------------------------------------------
    def execute(self, line: str, timeout: float | None = None) -> Response:
        return self.s.execute(line, timeout=timeout)

    def call(
        self,
        command: str,
        *params: int | float | str,
        text: str | None = None,
        timeout: float | None = None,
    ) -> Response:
        """Build and run an RPN invocation.

        Numeric params are pushed first, then the command word, then any
        trailing text the command will parse for itself.
        """
        words = [self._fmt(p) for p in params]
        words.append(command)
        if text is not None:
            words.append(text)
        return self.execute(" ".join(words), timeout=timeout)

    def expect_ok(self, command: str, *params, text=None, timeout=None) -> Response:
        resp = self.call(command, *params, text=text, timeout=timeout)
        if not resp.ok:
            raise AssertionError(f"{command!r} failed: {self._diag(resp)}")
        return resp

    def expect_error(self, line: str, timeout: float | None = None) -> Response:
        """Assert the interpreter rejected this input and stayed alive.

        Staying alive is the part that matters. A CLI that faults on bad
        input is unusable for field debugging, which is the whole point of
        having it.
        """
        resp = self.execute(line, timeout=timeout)
        if resp.timed_out:
            raise AssertionError(f"{line!r} produced no response; interpreter may be stuck")
        if not resp.errors:
            raise AssertionError(f"{line!r} was accepted but should have been rejected")
        return resp

    # -- parameter stack --------------------------------------------------
    def stack(self) -> StackSnapshot:
        """Full snapshot: depth and every value."""
        resp = self.call(self.cfg.stack_cmd)
        if not resp.ok:
            raise StackError(f"{self.cfg.stack_cmd} failed: {self._diag(resp)}")
        return parse_stack_output(
            resp.text, self.cfg.stack_format, self.cfg.stack_display_order
        )

    def depth(self) -> int:
        return self.stack().depth

    def clear_stack(self) -> None:
        self.call(self.cfg.clear_stack_cmd)

    def reset(self) -> None:
        """Full interpreter reset. The isolation primitive between tests.

        Sent unframed and deliberately. `resetcli` stops interpreting and
        resets the input parser, so a framed send is unsafe on two counts:
        anything after it on the line is discarded, and buffered input -
        potentially including the harness's own end marker - may be thrown
        away. So it goes out alone, the output is drained, and the link is
        resynced before anything else is sent.
        """
        self.s.raw(self.cfg.reset_cmd + self.cfg.tx_eol,
                   quiet_for=self.cfg.reset_settle)
        self._specs = None  # macros are gone, so the registry is stale

    def assert_baseline(self, context: str = "") -> None:
        snap = self.stack()
        if snap.depth != self.cfg.baseline_depth:
            self.clear_stack()
            raise StackError(
                f"parameter stack left at {snap} (expected depth "
                f"{self.cfg.baseline_depth}){' after ' + context if context else ''}. "
                "Stack was cleared; the command under test leaks or consumes "
                "the wrong number of parameters."
            )

    # -- sentinels --------------------------------------------------------
    def push_sentinels(self, count: int) -> tuple[int, ...]:
        """Push `count` marked values and return them, bottom first.

        Use these as a floor under the real parameters. Anything that
        disturbs them reached below its own arguments.
        """
        values = tuple(sentinel(i + 1, self.cfg.sentinel_base) for i in range(count))
        if values:
            self.execute(" ".join(str(v) for v in values))
        return values

    def assert_sentinels_intact(self, expected: tuple[int, ...], context: str) -> None:
        """The sentinels must be present, in order, at the bottom.

        Failing this means the command reached past its declared arity. It
        is a far more specific finding than "the stack looks wrong", and it
        is the check an empty-stack underflow test cannot make.
        """
        snap = self.stack()
        found = snap.sentinels(self.cfg.sentinel_base)
        if found != expected:
            self.clear_stack()
            raise StackError(
                f"{context}: sentinel floor disturbed.\n"
                f"  expected: {[f'S{v & 0xFFFF}' for v in expected]}\n"
                f"  found:    {[f'S{v & 0xFFFF}' for v in found]}\n"
                f"  stack:    {snap}\n"
                "The command consumed values it did not declare, or popped "
                "before checking depth."
            )

    @contextlib.contextmanager
    def sentinel_floor(self, count: int = 2, context: str = ""):
        """Lay a marked floor, run the block, verify the floor survived."""
        self.clear_stack()
        marks = self.push_sentinels(count)
        try:
            yield marks
        finally:
            pass
        self.assert_sentinels_intact(marks, context or "sentinel_floor")
        self.clear_stack()

    # -- macros -----------------------------------------------------------
    @contextlib.contextmanager
    def macro(self, name: str, *body: str):
        """Define a macro for the duration of the block, then forget it.

        Collapses repeated multi-word sequences into one token. At 115200
        that is a real saving across a sweep, and it keeps multi-step
        fixtures readable.

        Cleanup matters: without forget, macros accumulate across a session
        and tests stop being independent.
        """
        text = " ".join(body)
        resp = self.execute(
            f"{self.cfg.macro_begin_cmd} {name} {text} {self.cfg.macro_end_cmd}"
        )
        if not resp.ok:
            raise ProtocolError(f"could not define macro {name!r}: {self._diag(resp)}")
        try:
            yield name
        finally:
            if self.cfg.macro_forget_cmd:
                self.execute(f"{self.cfg.macro_forget_cmd} {name}")

    # -- introspection ----------------------------------------------------
    def word_names(self, refresh: bool = False) -> list[str]:
        resp = self.call(self.cfg.words_cmd, timeout=max(self.cfg.command_timeout, 5.0))
        if not resp.ok:
            raise ProtocolError(f"word list failed: {self._diag(resp)}")
        return sorted({w for line in resp.lines for w in line.split()})

    def describe(self, name: str) -> CommandSpec:
        resp = self.call(self.cfg.describe_cmd, text=name)
        if not resp.ok:
            return CommandSpec(name=name, raw=resp.text)
        return parse_spec(name, resp.text)

    def commands(self, refresh: bool = False) -> dict[str, CommandSpec]:
        """Full registry, fetched from the target itself.

        A bare `help` (no filter) lists every word with its one-line help
        in a single response, replacing one `help <name>` round trip per
        word - at ~165 real commands, each paying its own marker-framing
        overhead, that per-word loop was the single most expensive thing
        this harness did every session. Cross-checked against
        word_names() (one more cheap round trip) so a name that's
        discoverable but somehow missing from the help dump still gets an
        entry - the same guarantee the old per-word loop gave, just not
        the primary source anymore.

        Discovering the command set at runtime is what keeps the test matrix
        honest: add a word to the firmware and the coverage test fails until
        someone writes a case for it.
        """
        if self._specs is None or refresh:
            resp = self.call(self.cfg.describe_cmd, timeout=max(self.cfg.command_timeout, 5.0))
            if not resp.ok:
                raise ProtocolError(f"bare {self.cfg.describe_cmd!r} failed: {self._diag(resp)}")
            specs: dict[str, CommandSpec] = {}
            for line in resp.lines:
                parts = line.split(None, 1)
                if not parts:
                    continue
                name = parts[0]
                specs[name] = parse_spec(name, line)

            names = self.word_names()
            # A command name that collides with cfg.async_prefix (e.g.
            # "!") has its own help line misclassified as an unsolicited
            # event by Session._is_event() - not a real event, just a name
            # starting with the same character - and lands in resp.events
            # instead of resp.lines. Recovered here rather than by
            # loosening event detection itself, which would risk real
            # events leaking into normal command responses. Matters more
            # than a missing help string: a command that falls through to
            # the blank CommandSpec(name=name) fallback below reads as
            # arity 0, which pulls it into the "safe to run bare" sweeps
            # with none of the real word's argument requirements - for a
            # word like "!" (a raw memory store), that's a command that
            # writes through whatever garbage is left on an underflowed
            # stack instead of being excluded like a properly-parsed
            # arity-2 word would be.
            for line in resp.events:
                parts = line.split(None, 1)
                if parts and parts[0] in names and parts[0] not in specs:
                    specs[parts[0]] = parse_spec(parts[0], line)

            for name in names:
                specs.setdefault(name, CommandSpec(name=name))
            self._specs = specs
        return self._specs

    # -- helpers ----------------------------------------------------------
    @staticmethod
    def _fmt(value: int | float | str) -> str:
        if isinstance(value, bool):
            return "1" if value else "0"
        if isinstance(value, int):
            return str(value)
        return str(value)

    @staticmethod
    def _diag(resp: Response) -> str:
        parts = []
        if resp.timed_out:
            parts.append("TIMEOUT")
        if resp.crc_failures:
            parts.append(f"{resp.crc_failures} CRC failure(s)")
        if resp.errors:
            parts.append("; ".join(resp.errors))
        if resp.lines:
            parts.append(f"output={resp.lines!r}")
        if resp.events:
            parts.append(f"events={resp.events!r}")
        return " | ".join(parts) or "no output"


# ----------------------------------------------------------------------
# This board's actual convention is a SINGLE dash, always surrounded by
# spaces: "( n a - )", "( n - -n )" for negate, "( - a )" for here. Checked
# every stack comment in WordLists/wordlist.txt and board-NanoR4's
# wordlist.c: zero of them contain "--". The separator regex matches "-" or
# "--" (both require a bounding space, so it can't fire in the middle of a
# result like "-n") so tools/fake_target.py's "( a b -- sum )" style, used
# for the host build, still parses too.
#
# Falls back to a bare spec rather than raising, so an unparseable help
# string degrades coverage instead of breaking the run.
#
# A handful of words - dac!, nap, tabto, set-utc, i2cpattern, ... - drop the
# dash entirely and write just "(n)" or "(limb start_channel)". Per the
# board's convention, the dash is only there to separate outputs from
# inputs, so it's only required when there ARE outputs: "(n)" means exactly
# what "(n -)" means. _ARGS_ONLY catches that case as a fallback once
# _STACK_EFFECT (which requires the dash) has already had a chance to match.
#
# It's deliberately narrow - only a space/word-character run is accepted
# inside the parens - so it can't misfire on a parenthesized aside that
# isn't a stack comment at all, e.g. i2cscan's "(0x08-0x77)": that has a
# bare, unspaced "-" in it, which is excluded from the character class, so
# it correctly falls through to "no declared effect" instead of being read
# as one argument named "0x08-0x77".
# ----------------------------------------------------------------------
_STACK_EFFECT = re.compile(r"\((?P<args>[^()]*?)\s--?\s(?P<results>[^()]*)\)")
_ARGS_ONLY = re.compile(r"\((?P<args>[\w ]+)\)")


def parse_spec(name: str, text: str) -> CommandSpec:
    spec = CommandSpec(name=name, raw=text)
    match = _STACK_EFFECT.search(text)
    if match:
        spec.args = match.group("args").split()
        spec.results = match.group("results").split()
        spec.help = text[match.end():].strip()
        spec.has_declared_effect = True
        return spec
    match = _ARGS_ONLY.search(text)
    if match:
        spec.args = match.group("args").split()
        spec.help = text[match.end():].strip()
        spec.has_declared_effect = True
        return spec
    spec.help = text.strip()
    return spec

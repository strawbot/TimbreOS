"""
Do the stack comments actually describe what the commands do?

Comments and code drift apart in every codebase. Here the drift is worse
than cosmetic: the declared arity drives the generated underflow tests, so a
wrong comment does not just mislead a reader, it silently weakens the suite.
These tests make the comment load-bearing.

Two tiers, chosen automatically per command:

  derivable   every result name is an argument name, so the comment fully
              specifies the behaviour and exact stack contents are checked
              (dup, drop, swap, over, and any shuffler added later)

  depth only  a result is a new name, so only the depth change is checkable
              (add, peek, and most real commands)
"""

import pytest

from timbre import check_effect, derive_expected, is_probe, probe

from tests.test_command_matrix import DESTRUCTIVE

# Commands whose arguments are ACTED ON, not merely moved. Probe values are
# meaningless as addresses, channel numbers, or durations, and on real
# hardware a probe passed to a memory word will fault.
#
# TODO: extend this. Anything taking an address, a handle, a pin, a channel,
# or a duration belongs here. Each still needs a hand-written test using
# valid values - the exclusion is about probe safety, not about coverage.
PROBE_UNSAFE = {
    "peek",
    "poke",
    "nap",
    "execute",
    "@",
    "!",
    "c@",
    "c!",
    "s@",
    "s!",
    "+b",
    "-b",
    "cmove",
    "fill",
    # strlen/type treat their argument as a real string/buffer address and
    # dereference it directly (stringLength()/type() in cli.c) - same
    # "acts on the value, doesn't just move it" hazard as @/c@/s@ above.
    # Confirmed live: a generic arity-filler sweep passed strlen a bare
    # "1" and segfaulted on strlen((char*)1).
    "strlen",
    "type",
    # allot MOVES the dictionary pointer by its argument. A probe or
    # sentinel is an arbitrary (and on a 32-bit target, often negative-
    # looking) count: before cli.c's allot() grew a lower bound, that moved
    # hp below here-space, and the next thing to write at hp (c, , a quoted
    # string, .h's number buffer) corrupted whatever memory sat there -
    # reproduced as a host segfault, and the suspected cause of a board
    # hang right after test_shufflers_match_their_comments in a full serial
    # run. Guarded now, but a sweep still has no business steering hp with
    # garbage; test_strings.test_allot_advances_and_rewinds_the_dictionary_
    # pointer covers it with real values.
    "allot",
    "erase",
    # DAC/I2C actuation on real limb hardware: a limb/channel argument is
    # an index into physical wiring, not just a number, and a probe or
    # sentinel value passed as one is a garbage limb number at best and a
    # real actuation command (i2cpattern, i2charness) at worst.
    "dac!",
    "limbio!",
    "limbio-assign1",
    "limbio-reset1",
    "i2cpattern",
    "i2charness",
    "i2ctest",
    "i2cregs",
    "i2cseq",
    "i2cverify",
    # (s) blocks for s seconds to measure ticks/timeouts. A sentinel here
    # is a multi-billion-second sleep, not a fast failure.
    "testtime",
    # >r/r>/r touch the RETURN stack, not a benign second data area - on
    # this interpreter it's the same return stack used for its own control
    # flow. Confirmed live: a generic sweep called bare ">r" (with plenty
    # of data-stack headroom - this is not an underflow case) to push one
    # value onto it, with nothing in the same line to balance it with a
    # matching "r>". The board went completely silent afterward - no
    # response even to a follow-up resetcli - and needed a physical reset.
    # r> (pops the return stack) is excluded for the mirror-image reason;
    # r (a non-destructive copy) is excluded too out of caution, since it
    # hasn't been verified safe and the failure mode for getting this
    # wrong is not "test fails", it's "board needs a physical reset".
    ">r",
    "r>",
    "r",
    # .r's second argument is a field width ("print m in right field of n
    # digits"), acted on as a real padding count - not a benign moved
    # value. Confirmed live: a sweep passed it a 32-bit sentinel
    # (2119630850) as n, and the board went unresponsive immediately
    # afterward for the rest of the session. Same shape of hazard as nap/
    # testtime's duration argument, just for a padding loop instead of a
    # sleep.
    ".r",
    # dump ( a n - ) reads real memory at an arbitrary address for n
    # 16-byte rows - both the peek/@ hazard (a is acted on as a pointer,
    # not moved) and the .r hazard (n is an unbounded row count) at once.
    # Confirmed live: a sweep called it with a=2119630849, n=2119630850.
    "dump",
}

# Commands that clear the stack as their purpose. A sentinel floor cannot
# survive them, so probing them says nothing. They are tested directly
# instead - see tests/test_reset.py.
PROBE_EXEMPT = {"resetcli", "sp!"}

# Syntactic particles: only meaningful inside a construct, so invoking one
# standalone correctly errors. They appear in the word list and some carry
# a stack comment (if, for, until, while all declare "( n - )"), but they
# are not commands in the sense this sweep assumes - probing them exercises
# compiler/control-flow state, not a stack effect.
#
# The board marks every one of these "[i]" in its own help text (immediate:
# executed rather than compiled into a macro), which is exactly the
# "context-only, not a plain command" property this sweep needs to exclude
# on - see CommandSpec.is_immediate. So nothing needs to be hand-listed
# here; ";" is covered the same way (its help is "[i] end a macro build")
# and is tested directly in tests/test_macros.py.

# Float words whose comment reuses an argument name for the result - e.g.
# sqrt ( f -- f ), f+ ( f m -- f ) - not because the command claims to
# hand the same VALUE back unchanged (dup/swap/over's actual claim), but
# because both slots hold the same TYPE (a float) and single-letter names
# are doing double duty as type labels. derive_expected() can't tell these
# two conventions apart from the text alone: by the "result name is an
# argument name" rule it reads sqrt as a claimed passthrough, and the
# derivable-tier sweep then expects sqrt(probe) to equal probe, which a
# square root will never do. Forced onto the depth-only tier instead via
# check_effect(..., derivable=False) - their arity is still checked, just
# not value identity. See test_float.py for the real behavioral tests.
FLOAT_TYPE_LABELED = {
    "f+", "f-", "f*", "f/", "fabs", "fneg", "pow", "atan2",
    "sqrt", "cbrt", "exp", "ln", "log",
    "sin", "cos", "tan", "asin", "acos", "atan",
}


def _testable(spec):
    return (
        spec.name not in DESTRUCTIVE
        and spec.name not in PROBE_UNSAFE
        and spec.name not in PROBE_EXEMPT
        and not spec.is_immediate
        and not spec.takes_text
        and spec.has_declared_effect
    )


def _is_derivable(spec):
    return (
        spec.name not in FLOAT_TYPE_LABELED
        and not spec.is_alternation
        and derive_expected(spec.args, spec.results) is not None
    )


# ----------------------------------------------------------------------
# Derivable: the comment is the test
# ----------------------------------------------------------------------
def test_shufflers_match_their_comments(cli, commands):
    """Exact contents for every command whose comment determines them.

    Covers dup, drop, swap and over with no hand-written cases, and picks up
    any shuffler added later the moment its comment exists.
    """
    checked, failures = [], []
    for name, spec in sorted(commands.items()):
        if not _testable(spec):
            continue
        if not _is_derivable(spec):
            continue
        result = check_effect(cli, spec)
        checked.append(name)
        if not result.ok:
            failures.append(f"  {result}\n      {result.reason}")

    cli.reset()
    assert not failures, (
        "commands whose behaviour contradicts their stack comment:\n"
        + "\n".join(failures)
    )
    assert checked, (
        "no command had a comment of the form ( a b -- b a ). Either the "
        "describe output is not parsing, or no pure stack words are exposed."
    )


# ----------------------------------------------------------------------
# Depth only: the comment is a partial test
# ----------------------------------------------------------------------
def test_declared_arity_matches_observed(cli, commands):
    """Depth change must match the comment, for everything probe-safe."""
    failures = []
    for name, spec in sorted(commands.items()):
        if not _testable(spec):
            continue
        if _is_derivable(spec):
            continue  # covered exactly above
        result = check_effect(cli, spec, derivable=False)
        if not result.ok:
            failures.append(f"  {result}\n      {result.reason}")

    cli.reset()
    assert not failures, "commands whose depth change contradicts their comment:\n" + "\n".join(
        failures
    )


def test_effect_report(cli, commands, record_property):
    """Declared vs observed for every command, as build metadata.

    Not an assertion. Useful when adopting the harness on an existing
    codebase: read the report, fix the comments that are wrong, then turn
    the assertions above on.
    """
    lines = []
    for name, spec in sorted(commands.items()):
        if not _testable(spec):
            lines.append(f"{name:<14} {spec.raw.strip() or '(no comment)'}  [skipped]")
            continue
        result = check_effect(cli, spec, derivable=_is_derivable(spec))
        verdict = "ok" if result.ok else f"MISMATCH: {result.reason}"
        kind = "exact" if result.derivable else "depth"
        lines.append(f"{name:<14} {spec.raw.strip():<40} [{kind}] {verdict}")
    cli.reset()
    report = "\n".join(lines)
    record_property("stack_effect_report", report)
    print("\n" + report)


# ----------------------------------------------------------------------
# Unit tests for the derivation itself
# ----------------------------------------------------------------------
@pytest.mark.parametrize(
    "args,results,expected",
    [
        (["n"], ["n", "n"], (probe(1), probe(1))),          # dup
        (["n"], [], ()),                                     # drop
        (["a", "b"], ["b", "a"], (probe(2), probe(1))),      # swap
        (["a", "b"], ["a", "b", "a"],                        # over
         (probe(1), probe(2), probe(1))),
        (["a", "b"], ["sum"], None),                         # not derivable
        ([], ["n"], None),                                   # result from nowhere
    ],
)
def test_derivation(args, results, expected):
    assert derive_expected(args, results) == expected


def test_probes_are_distinguishable_from_sentinels(cfg):
    """The two marker families must not collide, or reports become nonsense."""
    from timbre import is_sentinel, sentinel

    for i in range(1, 8):
        assert is_probe(probe(i)) and not is_sentinel(probe(i), cfg.sentinel_base)
        assert not is_probe(sentinel(i, cfg.sentinel_base))

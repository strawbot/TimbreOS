"""
Float words (WordLists/floatwords.txt).

Floats share the integer parameter stack - a float is just the IEEE-754
bit pattern of a 32-bit C `float` sitting in the same Cell-sized slot an
integer would occupy (TimbreOS/float.c: getFloat/putFloat are a plain
union reinterpret, no tagging anywhere). Nothing on the stack marks a
slot as "this one's a float" - a float word assumes whatever it pops
already is one, and leaves a float in its place. f>i/i>f are the only
words that actually convert the representation; verified directly here
by pushing an int and a float side by side and reading the raw stack
(test_float_and_int_share_one_stack).

A literal float is written as digits with exactly one decimal point
(cli.c's signDigits(), #ifdef FLOAT_SUPPORT); everything after the point
is the fractional part, and there's no exponent notation. Values round to
32-bit float precision on the way in (per the project's own documented
invariant: 8.95 stores as ~8.9499998 - expected, not a bug), and `.f`
prints with a fixed 4 decimal places (float.c: `decimals = 4`) - enough
to hide most of that imprecision but not guaranteed to for every value,
so comparisons here go through a tolerance (assert_float_close) rather
than exact string equality, and stick to values that land cleanly at 4
decimal places (halves, quarters, small integers) wherever the exact
printed result matters.
"""

import struct

import pytest


def to_f32(x: float) -> float:
    """Round-trip through a 32-bit float, matching this CLI's precision."""
    return struct.unpack("f", struct.pack("f", x))[0]


def read_float(cli) -> float:
    """`.f` the top of stack and parse the printed result back to a float.

    Pops the value, same as the CLI does - callers push first.
    """
    resp = cli.execute(".f")
    assert resp.ok, f".f failed: {resp.text!r} {resp.errors}"
    return float(resp.text.strip())


def assert_float(cli, text: str, expected: float, *, tol: float = 5e-4):
    """Run `text` (which must leave exactly one float on the stack), then
    check it against `expected` (rounded to 32-bit float precision first,
    since that's the precision the CLI itself can represent).
    """
    resp = cli.execute(text)
    assert resp.ok, f"{text!r} failed: {resp.text!r} {resp.errors}"
    actual = read_float(cli)
    assert actual == pytest.approx(to_f32(expected), abs=tol), (
        f"{text!r}: expected ~{to_f32(expected)}, got {actual}"
    )


# ----------------------------------------------------------------------
# Literals and representation
# ----------------------------------------------------------------------
def test_float_literal_pushes_one_item(cli, cfg):
    cli.reset()
    try:
        before = cli.depth()
        cli.execute("3.5")
        assert cli.depth() == before + 1
    finally:
        cli.reset()


def test_dot_f_prints_and_consumes(cli, cfg):
    cli.reset()
    try:
        before = cli.depth()
        resp = cli.execute("3.5 .f")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert cli.depth() == before  # .f pops what it prints
    finally:
        cli.reset()


def test_dot_f_prints_four_decimal_places(cli, cfg):
    cli.reset()
    try:
        resp = cli.execute("1.5 .f")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert resp.text.strip() == "1.5000"
    finally:
        cli.reset()


def test_float_and_int_share_one_stack(cli, cfg):
    """No separate float stack: an int and a float sit side by side, and
    `.s` shows the float as its raw bit pattern - the same representation
    float words themselves read via getFloat()'s union reinterpret.
    """
    cli.reset()
    try:
        resp = cli.execute("5 3.5 .s")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        snap = cli.stack()
        assert snap.depth == 2
        assert snap.values[0] == 5
        assert snap.values[1] == struct.unpack("I", struct.pack("f", 3.5))[0]
    finally:
        cli.reset()


# ----------------------------------------------------------------------
# i>f / f>i - the only words that change representation
# ----------------------------------------------------------------------
def test_i_to_f_produces_a_float(cli, cfg):
    cli.reset()
    try:
        resp = cli.execute("5 i>f .f")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert resp.text.strip() == "5.0000"
    finally:
        cli.reset()


def test_i_to_f_and_f_to_i_round_trip(cli, cfg):
    cli.reset()
    try:
        resp = cli.execute("5 i>f f>i")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert cli.stack().top == 5
    finally:
        cli.reset()


@pytest.mark.parametrize("literal,expected", [
    ("3.2", 3), ("3.5", 4), ("3.7", 4), ("-3.2", -3), ("-3.5", -4), ("-3.7", -4),
])
def test_f_to_i_rounds_rather_than_truncates(cli, cfg, literal, expected):
    """float.c cliFtoi() adds +-0.5 before casting - documented, non-obvious
    behavior worth pinning: naive truncation would give 3/-3 throughout.
    """
    cli.reset()
    try:
        resp = cli.execute(f"{literal} f>i")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert cli.stack().top == expected, f"{literal} f>i: expected {expected}"
    finally:
        cli.reset()


# ----------------------------------------------------------------------
# Arithmetic - exactly-representable operands so results land clean
# ----------------------------------------------------------------------
def test_f_plus(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "1.5 2.5 f+", 4.0)
    finally:
        cli.reset()


def test_f_minus(cli, cfg):
    """f- ( f m -- f ): subtracts the top (m) from the one below (f)."""
    cli.reset()
    try:
        assert_float(cli, "3.0 2.0 f-", 1.0)
    finally:
        cli.reset()


def test_f_star(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "2.0 3.0 f*", 6.0)
    finally:
        cli.reset()


def test_f_slash(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "6.0 2.0 f/", 3.0)
    finally:
        cli.reset()


def test_fabs(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "-2.5 fabs", 2.5)
    finally:
        cli.reset()


def test_fneg(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "2.5 fneg", -2.5)
    finally:
        cli.reset()


# ----------------------------------------------------------------------
# f> - the one comparison word; result is a plain int flag, not a float
# ----------------------------------------------------------------------
def test_f_greater_true(cli, cfg):
    cli.reset()
    try:
        resp = cli.execute("3.0 2.0 f>")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert cli.stack().top == 1
    finally:
        cli.reset()


def test_f_greater_false(cli, cfg):
    cli.reset()
    try:
        resp = cli.execute("2.0 3.0 f>")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert cli.stack().top == 0
    finally:
        cli.reset()


# ----------------------------------------------------------------------
# Transcendental - values chosen so the exact result is representable
# ----------------------------------------------------------------------
def test_pi(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "pi", 3.14159265)
    finally:
        cli.reset()


def test_sqrt(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "4.0 sqrt", 2.0)
    finally:
        cli.reset()


def test_cbrt(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "8.0 cbrt", 2.0)
    finally:
        cli.reset()


def test_pow(cli, cfg):
    """pow ( f m -- f ): f to the power of m."""
    cli.reset()
    try:
        assert_float(cli, "2.0 3.0 pow", 8.0)
    finally:
        cli.reset()


def test_exp(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "0.0 exp", 1.0)
    finally:
        cli.reset()


def test_ln(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "1.0 ln", 0.0)
    finally:
        cli.reset()


def test_log(cli, cfg):
    """log is base 10, per its own help text."""
    cli.reset()
    try:
        assert_float(cli, "100.0 log", 2.0)
    finally:
        cli.reset()


def test_sin_cos_tan_at_zero(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "0.0 sin", 0.0)
        assert_float(cli, "0.0 cos", 1.0)
        assert_float(cli, "0.0 tan", 0.0)
    finally:
        cli.reset()


def test_asin_acos_atan(cli, cfg):
    cli.reset()
    try:
        assert_float(cli, "1.0 asin", 1.5707963)   # pi/2
        assert_float(cli, "0.0 acos", 1.5707963)   # pi/2
        assert_float(cli, "1.0 atan", 0.7853982)   # pi/4
    finally:
        cli.reset()


def test_atan2(cli, cfg):
    """atan2 ( f m -- m ): inverse tan of f/m, i.e. atan2(f, m)."""
    cli.reset()
    try:
        assert_float(cli, "1.0 1.0 atan2", 0.7853982)   # pi/4
        assert_float(cli, "0.0 1.0 atan2", 0.0)
    finally:
        cli.reset()

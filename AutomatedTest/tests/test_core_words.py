"""
Hand-written behavior tests for the core word set: arithmetic and logic,
comparison, number printing and formatting, memory, control flow, constants
and variables, the return stack.

Every case runs on one framed line and asserts the whole stack (index 0 is
the bottom) or the whole printed text, so a wrong result, a wrong depth, and
a stray print all fail the same way. Values are chosen to mean the same
thing on a 32-bit target and on the 64-bit host build (Cell is uintptr_t):
no raw `not` results, no overflow, no values that only fit in 32 bits.

Memory words work on scratch that is always safe: a `variable`'s own cell,
or bytes taken from here-space with `here N allot`. Nothing here writes to
an address the test did not just allocate.

A target that lacks a word (tools/fake_target.py implements only a subset)
skips the test rather than failing it; the coverage gate, not this file,
decides whether a command has to exist.
"""

import re

import pytest

from tests.test_command_matrix import COMPILE_ONLY


def _is_number(token: str) -> bool:
    try:
        int(token, 0)
        return True
    except ValueError:
        return False


def _need(cli, line: str) -> None:
    """Skip unless the target has every word `line` uses.

    Names the line itself defines (`variable v`, `constant seven`, `: t`)
    are not target words, and neither are numbers.
    """
    have = cli.commands()
    defined = set()
    needed = []
    tokens = line.split()
    in_comment = False
    for i, tok in enumerate(tokens):
        if in_comment:  # `( ... )` text is not words; no ")" means to end of line
            in_comment = not tok.endswith(")")
            continue
        if tok == "(":
            in_comment = True
            needed.append(tok)
            continue
        if i and tokens[i - 1] in ("variable", "constant", ":"):
            defined.add(tok)
            continue
        if _is_number(tok) or tok in defined:
            continue
        needed.append(tok)
    missing = sorted({t for t in needed if t not in have})
    if missing:
        pytest.skip(f"target has no {missing}")


def _run(cli, line: str):
    """Run one line on a clean stack; return (stack values bottom first, response)."""
    _need(cli, line)
    cli.clear_stack()
    resp = cli.execute(line)
    assert resp.ok and not resp.errors, f"{line!r} failed: {resp.text!r} {resp.errors}"
    values = list(cli.stack().values)
    cli.clear_stack()  # the isolation fixture fails a test that leaves values behind
    return values, resp


def _cell_bytes(cli) -> int:
    """sizeof(Cell) on the target: .h pads to sizeof(Cell)*2 hex digits."""
    _need(cli, "255 .h")
    cli.clear_stack()
    resp = cli.execute("255 .h")
    assert resp.ok, resp.errors
    cli.clear_stack()
    width = len(resp.text.rstrip("\n")) - 1  # doth() ends with one space
    assert width in (8, 16), f"unexpected .h field width {width}: {resp.text!r}"
    return width // 2


# ----------------------------------------------------------------------
# Arithmetic, logic, comparison
# ----------------------------------------------------------------------
@pytest.mark.parametrize(
    "line, expected",
    [
        ("5 3 +", [8]),
        ("5 3 -", [2]),
        ("6 7 *", [42]),
        ("7 2 /", [3]),
        ("7 2 mod", [1]),
        # remainder below, quotient on top (cli.c: "remainder \ quotient")
        ("7 2 /mod", [1, 3]),
        ("12 10 and", [8]),
        ("12 10 or", [14]),
        ("12 10 xor", [6]),
        # `not` flips every bit, and how many bits there are depends on the
        # target's Cell, so look at the low byte only.
        ("5 not 255 and", [250]),
        ("5 negate", [-5]),
        ("-7 abs", [7]),
        ("7 abs", [7]),
        ("3 9 max", [9]),
        ("9 3 max", [9]),
        ("3 9 min", [3]),
        ("9 3 min", [3]),
        ("1 4 shift", [16]),
        ("16 -2 shift", [4]),
        ("2 3 <", [1]),
        ("3 2 <", [0]),
        ("-1 1 <", [1]),
        ("3 3 =", [1]),
        ("3 4 =", [0]),
        ("4 3 >", [1]),
        ("3 4 >", [0]),
        ("0 ?dup", [0]),
        ("5 ?dup", [5, 5]),
    ],
)
def test_arithmetic_logic_and_comparison(cli, line, expected):
    values, _ = _run(cli, line)
    assert values == expected, f"{line!r}: expected {expected}, got {values}"


@pytest.mark.parametrize(
    "line, expected",
    [("1 0 /", [0]), ("5 0 mod", [0]), ("7 0 /mod", [0, 0])],
)
def test_division_by_zero_is_reported_and_survivable(cli, line, expected):
    """Reports div/0 (or mod/0), leaves a defined result, and keeps running."""
    _need(cli, line)
    cli.clear_stack()
    resp = cli.execute(line)
    assert not resp.timed_out, f"{line!r} hung"
    assert resp.errors, f"{line!r} divided by zero without saying so"
    assert list(cli.stack().values) == expected
    cli.clear_stack()
    values, _ = _run(cli, "1 2 +")
    assert values == [3], "interpreter not usable after a divide by zero"


# ----------------------------------------------------------------------
# Printing, number bases, number formatting
# ----------------------------------------------------------------------
@pytest.mark.parametrize(
    "line, text",
    [
        ("42 .", "42"),
        ("255 .d", "255"),
        ("255 hex . decimal", "FF"),
        ("hex 10 .d decimal", "16"),
        ("oct 10 .d decimal", "8"),
        ("bin 101 .d decimal", "5"),
        ("decimal 10 .d", "10"),
        ("5 .b", "00000101"),
        # leading all-zero bytes are skipped, the rest print a byte at a time
        ("300 .b", "00000001 00101100"),
        ("65 emit", "A"),
    ],
)
def test_printing_words(cli, line, text):
    values, resp = _run(cli, line)
    assert resp.text.strip() == text, f"{line!r}: expected {text!r}, got {resp.text!r}"
    assert values == [], f"{line!r} left {values} on the stack"


def test_dot_r_right_justifies_in_a_field(cli):
    values, resp = _run(cli, "4 3 .r")
    assert resp.text.rstrip("\n") == "  4", f"got {resp.text!r}"
    assert values == []


@pytest.mark.parametrize(
    "line, text",
    [
        ("255 <# #s #> type", "255"),
        # # converts ONE digit: two digits of 123 is "23"
        ("123 <# # # #> type", "23"),
        # hold takes a character code; 45 is '-'
        ("45 <# #s 45 hold #> type", "-45"),
        # sign ( m n - n ) prepends '-' when m is negative
        ("-5 5 <# #s sign #> type", "-5"),
        ("5 5 <# #s sign #> type", "5"),
    ],
)
def test_number_formatting_sequence(cli, line, text):
    values, resp = _run(cli, line)
    assert resp.text.strip() == text, f"{line!r}: expected {text!r}, got {resp.text!r}"
    assert values == [], f"{line!r} left {values} on the stack"


def test_hash_s_leaves_zero(cli):
    """#s converts every digit, so what is left of the number is 0."""
    values, _ = _run(cli, "255 <# #s")
    assert values == [0]


def test_decimals_sets_float_places(cli):
    """decimals is a variable address: `2 decimals !` makes .f print 2 places."""
    _need(cli, "decimals @ 2 decimals ! pi .f decimals !")
    # Put it back afterwards: this is shared interpreter state.
    values, resp = _run(cli, "decimals @ 2 decimals ! pi .f decimals ! decimals @")
    assert "3.14" in resp.text and "3.141" not in resp.text, f"got {resp.text!r}"
    assert len(values) == 1, values  # the restored original, unchanged


# ----------------------------------------------------------------------
# Constants, variables, memory
# ----------------------------------------------------------------------
def test_constant(cli):
    values, _ = _run(cli, "7 constant seven seven seven +")
    assert values == [14]


def test_variable_fetch_and_store(cli):
    values, _ = _run(cli, "5 variable v v @ 9 v ! v @")
    assert values == [5, 9]


def test_byte_and_short_fetch_and_store(cli):
    values, _ = _run(cli, "5 variable v 6 v c! v c@ 258 v s! v s@")
    assert values == [6, 258]


def test_comma_and_c_comma_allocate_and_store(cli):
    cb = _cell_bytes(cli)
    values, _ = _run(cli, f"7 , here {cb} - @")
    assert values == [7], "`,` did not store its value in the cell it allocated"
    values, _ = _run(cli, "65 c, here 1 - c@")
    assert values == [65], "`c,` did not store its byte"
    values, _ = _run(cli, "here 9 c, here swap -")
    assert values == [1], "`c,` should advance here by exactly one byte"
    values, _ = _run(cli, "here 9 , here swap -")
    # `,` aligns here first, so the advance is a cell plus 0..3 bytes of padding
    assert len(values) == 1 and cb <= values[0] <= cb + 3, values


def test_fill_erase_and_cmove(cli):
    # buf = 8 bytes of here-space. fill ( s n x ), erase ( s n ), cmove ( s d n )
    setup = "here 8 allot variable buf buf @ 8 65 fill"
    values, _ = _run(cli, f"{setup} buf @ c@ buf @ 7 + c@")
    assert values == [65, 65], "fill did not fill every byte"
    values, _ = _run(cli, f"{setup} buf @ 4 + 4 erase buf @ 3 + c@ buf @ 4 + c@ buf @ 7 + c@")
    assert values == [65, 0, 0], "erase touched the wrong bytes"
    copy = (
        "here 8 allot variable buf buf @ 8 0 fill "
        "1 buf @ c! 2 buf @ 1 + c! 3 buf @ 2 + c! "
        "buf @ buf @ 4 + 3 cmove buf @ 4 + c@ buf @ 5 + c@ buf @ 6 + c@ buf @ 7 + c@"
    )
    values, _ = _run(cli, copy)
    assert values == [1, 2, 3, 0], "cmove copied the wrong bytes"


def test_plus_b_and_minus_b_set_and_clear_bits(cli):
    # +b ( bits addr ) ORs bits in, -b clears them.
    line = (
        "here 8 allot variable buf buf @ 8 0 fill "
        "5 buf @ +b buf @ c@ "
        "1 buf @ -b buf @ c@"
    )
    values, _ = _run(cli, line)
    assert values == [5, 4]


def test_dump_prints_address_hex_and_ascii(cli):
    line = "here 16 allot variable buf buf @ 16 65 fill buf @ 1 dump"
    _need(cli, line)
    cli.clear_stack()
    resp = cli.execute(line)
    assert resp.ok and not resp.errors, resp.errors
    assert re.search(r": 41 41 41 41 41 41 41 41 +41 41 41 41 41 41 41 41", resp.text), resp.text
    assert "AAAAAAAA AAAAAAAA" in resp.text, resp.text
    assert cli.depth() == 0, "dump left something on the stack"


def test_execute_runs_a_word_given_its_token(cli):
    """A variable's body is [ii | value] and `here` is right past it, so its
    execution token is here minus two cells; executing it pushes the address
    of its value."""
    cb = _cell_bytes(cli)
    values, _ = _run(cli, f"5 variable v here {2 * cb} - execute @")
    assert values == [5]


# ----------------------------------------------------------------------
# Comments, macro-build brackets
# ----------------------------------------------------------------------
def test_comment_skips_to_the_closing_paren(cli):
    values, _ = _run(cli, "1 ( two 99 ) 2")
    assert values == [1, 2]


def test_comment_without_a_closing_paren_runs_to_end_of_line(cli):
    values, _ = _run(cli, "1 ( the rest is a comment 2 3")
    assert values == [1]


def test_left_bracket_leaves_macro_build_mode(cli):
    """`] [` enters and leaves macro build on one line; what follows runs."""
    values, _ = _run(cli, "] [ 1 2 +")
    assert values == [3]
    values, _ = _run(cli, "4 5 +")
    assert values == [9], "interpreter was left in macro build mode"


# ----------------------------------------------------------------------
# Control flow, inside a macro (where it is allowed to exist)
# ----------------------------------------------------------------------
@pytest.mark.parametrize(
    "line, expected",
    [
        (": t 3 for 7 next ; t", [7, 7, 7]),
        (": t 0 begin 1 + dup 4 = until ; t", [4]),
        (": t 0 begin dup 3 < while 1 + repeat ; t", [3]),
        (": t 9 begin exit again ; t", [9]),
        (": s 0 < if -1 else 1 endif ; -5 s 5 s", [-1, 1]),
        (": t if 11 endif ; 1 t 0 t", [11]),
    ],
)
def test_control_flow_in_a_macro(cli, line, expected):
    values, _ = _run(cli, line)
    assert values == expected


def test_return_stack_words_inside_a_macro(cli):
    values, _ = _run(cli, ": t 5 >r r r> ; t")
    assert values == [5, 5], ">r r r> inside a macro: expected [5, 5]"


# ----------------------------------------------------------------------
# Words that only make sense inside a definition are rejected, not run,
# at the command line - and the interpreter survives being asked.
# ----------------------------------------------------------------------
@pytest.mark.parametrize("word", sorted(COMPILE_ONLY | {">r", "r>", "r"}))
def test_definition_only_words_are_rejected_at_the_command_line(cli, word):
    _need(cli, word)
    cli.clear_stack()
    resp = cli.execute(word)
    assert not resp.timed_out, f"bare {word!r} hung the interpreter"
    assert resp.errors, f"bare {word!r} ran instead of being rejected: {resp.text!r}"
    values, _ = _run(cli, "1 2 +")
    assert values == [3], f"interpreter unusable after a bare {word!r}"

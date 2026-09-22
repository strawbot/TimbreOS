"""
Quoted-string literals and the words that print them.

These are load-bearing: every framed command in the suite already runs
config.mark_template's `"<marker>" dup dup strlen type cr here - allot` to
print its begin/end markers (see Session._mark_line), so `"..."`, `strlen`,
`type`, `cr`, `here` and `allot` execute on every single test. This file
gives them their own hand-written cases so the coverage gate counts that as
real coverage rather than an accident of plumbing.

here/allot earned their place in the template the hard way: a plain
`dup strlen type cr` prints the marker but never reclaims the dictionary
space `"..."` allot()s for it, and a long run sends thousands of these -
confirmed live, that eventually runs the dictionary into whatever memory
follows it. `here - allot` right after printing computes the negative of
the string's own length and allot()s it straight back, so the template
is dictionary-neutral no matter how many commands a session sends.
"""


def test_quote_pushes_one_address(cli):
    """`"text"` is one stack item: the address of the stored text."""
    cli.reset()
    try:
        before = cli.depth()
        cli.execute('"probe"')
        assert cli.depth() == before + 1
    finally:
        cli.reset()


def test_strlen_reports_the_length_and_consumes_the_address(cli):
    cli.reset()
    try:
        resp = cli.execute('"hello" strlen')
        assert resp.ok, f"strlen failed: {resp.text!r} {resp.errors}"
        assert cli.stack().top == 5
        assert cli.depth() == 1  # strlen replaces the address, doesn't add
    finally:
        cli.reset()


def test_type_prints_the_string_and_leaves_the_stack_clean(cli):
    cli.reset()
    try:
        before = cli.depth()
        resp = cli.execute('"hello" dup strlen type cr')
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert "hello" in resp.text
        assert cli.depth() == before  # dup+strlen+type is net stack-neutral
    finally:
        cli.reset()


def test_type_prints_only_the_requested_count(cli):
    """`n type` prints n characters starting at the address, not the whole string."""
    cli.reset()
    try:
        resp = cli.execute('"hello" dup 2 type drop cr')
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert resp.text.strip() == "he"
    finally:
        cli.reset()


def test_cr_emits_a_newline_without_touching_the_stack(cli):
    cli.reset()
    try:
        before = cli.depth()
        resp = cli.execute("cr")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert cli.depth() == before
    finally:
        cli.reset()


def test_quoted_string_survives_embedded_spaces(cli):
    """The marker template relies on this: the framing text itself has no
    spaces, but a command under test may legitimately quote one that does.
    """
    cli.reset()
    try:
        resp = cli.execute('"two words" dup strlen type cr')
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert "two words" in resp.text
    finally:
        cli.reset()


def test_here_returns_the_dictionary_pointer(cli):
    cli.reset()
    try:
        before = cli.depth()
        resp = cli.execute("here")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        assert cli.depth() == before + 1
    finally:
        cli.reset()


def test_dot_h_prints_a_hex_value_and_consumes_it(cli):
    """`here .h` is the harness's own leak monitor (see conftest.py's
    here_space_snapshot) - confirm .h actually prints what's on top,
    in hex, and leaves nothing behind.
    """
    cli.reset()
    try:
        before = cli.depth()
        resp = cli.expect_ok(".h", 255)
        # doth() space-pads to sizeof(Cell)*2 via dotr() (cli.c) - not
        # zero-padded, so "FF" is "      FF" (6 leading spaces), not
        # "000000FF".
        assert resp.text.strip().upper() == "FF", f"expected FF, got {resp.text!r}"
        assert resp.text.upper().startswith("      FF"), f"expected space-padding, got {resp.text!r}"
        assert cli.depth() == before
    finally:
        cli.reset()


def _here(cli) -> int:
    """`here` is a silent stack push, not a printed value - peek the
    stack for it and clear back down so repeated calls don't accumulate.
    Equivalent to (and cross-checked against, in the test below) the
    `here .h` form the leak monitor actually uses.
    """
    cli.expect_ok("here")
    value = cli.stack().top
    cli.clear_stack()
    return value


def test_here_dot_h_agrees_with_here_peeked_off_the_stack(cli):
    cli.reset()
    try:
        peeked = _here(cli)
        resp = cli.execute("here .h")
        assert resp.ok, f"failed: {resp.text!r} {resp.errors}"
        printed = int(resp.text.strip(), 16)
        assert printed == peeked
    finally:
        cli.reset()


def test_allot_advances_and_rewinds_the_dictionary_pointer(cli):
    """The exact mechanism the marker template depends on: allot(n) moves
    `here` forward by n, and allot(-n) moves it back - so creating a
    string and then allot()ing the negative of its own length returns the
    dictionary to exactly where it started.
    """
    cli.reset()
    try:
        start = _here(cli)
        cli.expect_ok("allot", 5)
        grown = _here(cli)
        assert grown == start + 5

        cli.expect_ok("allot", -5)
        back = _here(cli)
        assert back == start
    finally:
        cli.reset()


def test_marker_template_does_not_leak_dictionary_space(cli, cfg):
    """The actual property the fix exists for: printing many markers in a
    row must not advance `here` at all, since a long test run sends
    thousands of them and none of it is reclaimed except by a full reset.
    """
    cli.reset()
    try:
        start = _here(cli)
        for _ in range(25):
            cli.expect_ok(cfg.stack_cmd)
        end = _here(cli)
        assert end == start, (
            f"here moved from {start} to {end} over 25 framed commands - "
            "the marker template is leaking dictionary space again"
        )
    finally:
        cli.reset()

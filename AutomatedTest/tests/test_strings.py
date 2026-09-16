"""
Quoted-string literals and the words that print them.

These are load-bearing: every framed command in the suite already runs
`"<marker>" dup strlen type cr` to print its begin/end markers (see
config.mark_template and Session._mark_line), so `"..."`, `strlen`, `type`
and `cr` execute on every single test. This file gives them their own
hand-written cases so the coverage gate counts that as real coverage rather
than an accident of plumbing.
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

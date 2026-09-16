# ActiveRobot / TimbreOS CLI test harness

A skeleton for automated testing against the TimbreOS command line
interpreter. Everything target-specific is in `timbre/config.py`; nothing
else contains a literal command name, prompt, or timeout.

```
pip install pytest pyserial

python tools/check_link.py --host    # bring-up check, fake target
python tools/check_link.py           # bring-up check, real board

pytest --transport=host              # host build of the CLI, no board
pytest --transport=serial            # real hardware
pytest --transport=serial -m hw      # hardware-only tests
```

Run `tools/check_link.py` before the suite on first contact with a board. It
walks the protocol layers in order and stops at the first one that fails, so
you get "error_pattern does not match" rather than forty pytest failures that
all trace back to one config line. It reports the config values it infers
from the target but does not write them, since guessing silently is how a
harness ends up confidently testing something other than what you think.

Port on this setup is `/dev/cu.usbserial-FTVGKHLB3`. It is an FTDI adapter,
so the suffix is the chip's EEPROM serial number rather than a USB port
position, and it stays put across reboots and ports. Use the `cu.` node, never
`tty.` — the tty node blocks waiting for carrier detect and hangs with no
error. The harness refuses a tty path outright and tells you the cu
equivalent.

Out of the box it runs against `tools/fake_target.py`, a stand-in that
implements the protocol contract. Point `config.host_binary` at the real
host build to replace it.

---

## Layers

```
tests/                  pytest, plus a matrix generated from the target
  timing.py             longest-execution-time instrument
  cli.py                RPN call construction, stack, introspection
session.py              marker framing, async demux, correlation
codec.py                text CRC
transport.py            serial | host subprocess
```

Each layer is only meaningful if the one below it is trusted, which is why
`tests/test_link.py` exists and why it should run first.

## How framing works

The CLI executes multiple words per line, left to right, and has a word that
prints text. That is enough to make the target delimit its own responses:

```
> echo TH-B0007 <command under test>
> echo TH-E0007
```

Everything between the two marker lines is that command's output. No sleeps,
no readline races, no guessing when a response is complete. Async events are
recognised by prefix rather than by timing, and a missing end marker means
the interpreter is genuinely stuck rather than merely slow.

The end marker goes on a **separate line** deliberately. See finding 1.

## The two automatic guards

Both live in `conftest.py` and run around every test at zero authoring cost.
Both catch failures that are invisible where they are caused and only
surface later somewhere unrelated.

**Isolation.** Every test is preceded by `resetcli` — macros removed, stacks
reset, naps cancelled, input parser restored. Clearing the parameter stack is
not enough: a leftover macro or a pending nap makes the next test depend on
execution order, which is the hardest kind of suite bug to find. Teardown
*reads* the stack before resetting it, since resetting first would destroy
the evidence the guard exists to report.

Because `resetcli` stops interpreting and resets the input parser, it is sent
unframed and followed by a resync. Framing it would be unsafe twice over:
anything after it on the line is discarded, including a trailing marker, and
buffered input may be thrown away. `tests/test_reset.py` pins all five of its
documented effects individually, and checks that framing recovers afterwards.

**Stack guard.** The parameter stack is shared mutable state that outlives a
command. A test that leaves a value behind silently changes the meaning of
every test after it. Because `.s` reports values as well as depth, the guard
compares a full snapshot rather than a count, and failure messages show what
was actually left behind.

> Do not call `clear_stack()` at the end of a test. It destroys the evidence
> before teardown can see it. Clear *between* iterations, never after the
> last one.

**Timing guard.** Hardware only. Arms the execution-time instrument before
each test and asserts no action exceeded its design limit afterwards. This
makes the TimbreOS non-blocking promise executable: every test in the suite
becomes a latency regression test, and a busy-wait added two years from now
fails whichever test happens to exercise it, with the action named.

## Stack display order

`.s` prints as `4 stack items: 1 2 3 2`, with the bottom of the stack on the
left and the top on the right.

This was settled by experiment rather than from the stack comments, because
using the comments would be circular — verifying them is what the
conformance sweep is for. `test_stack_display_order` pushes three distinct
values, drops the top one, and sees which end lost a value:

```
11 22 33 drop  ->  "11 22"   rightmost was the top   (bottom_first)
               ->  "22 11"   leftmost was the top    (top_first)
```

Worth doing, because getting it wrong produces no error at all. It silently
inverts every stack assertion in the suite. Set deliberately wrong, the
symptom is that `dup` still passes — its result is symmetric — while `swap`
and `over` fail, which reads exactly like two broken shufflers rather than
one wrong config line. The discriminator names the setting and the value to
change it to.

`config.stack_display_order` handles both conventions; the parser normalises
so that index 0 is always the bottom and `.top` is always the top. Nothing
above the parser needs to know.

`test_display_order_matches_the_documented_examples` keeps these two
transcripts as a regression:

```
ar: 1 2 3 over .s   ->  4 stack items: 1 2 3 2
ar: 1 2 3 swap .s   ->  3 stack items: 1 3 2
```

## Stack comments as executable specifications

Comments and code drift apart everywhere. Here the drift is worse than
cosmetic: declared arity drives the generated underflow tests, so a wrong
comment silently weakens the suite rather than merely misleading a reader.

The leverage: when every result name is drawn from the argument names, the
comment does not describe the command, it **specifies** it.

```
dup  ( n -- n n )      push one probe, expect it twice
drop ( n -- )          push one probe, expect nothing
swap ( a b -- b a )    push two probes, expect them reversed
over ( a b -- a b a )  push two, expect three in that arrangement
```

So the assertions are derived from the comments. All four of TimbreOS's stack
words are covered with no hand-written cases, and a shuffler added later is
covered the moment its comment exists. Verified against both failure
directions — a buggy `over` that copies the top instead of the second, and a
correct `swap` with a falsified comment:

```
over ( a b -- a b a )   expected [P1 P2 P1], got [P1 P2 P2]
swap ( a b -- a b )     expected [P1 P2],    got [P2 P1]
```

Where a result is a new name (`add ( a b -- sum )`), values cannot be derived
and only the depth change is checked. `test_effect_report` prints declared vs
observed for every command — useful when adopting this on existing firmware:
read the report, fix the comments that are wrong, then turn the assertions on.

Three exemptions, each for a different reason and each still needing its own
hand-written test: `PROBE_UNSAFE` for commands that act on their arguments
rather than moving them (a probe passed to a memory word will fault on real
hardware), `PROBE_EXEMPT` for commands whose job is clearing the stack, and
`SYNTAX_ONLY` for particles like `;` that are only meaningful inside a
construct.

Note that text arguments do not appear in the stack comment, because they are
read from the input stream rather than popped. `: ( -- ) <name>` declares no
stack effect and still consumes the next word, so the harness detects text
commands by the `<...>` marker in the help line.

## Sentinels: testing underflow properly

Calling a command with an empty stack tells you only that it errored. It
cannot tell you whether the command checked depth **before** popping or
after. Both report the error correctly. Only the second leaves the stack
corrupted, and with nothing underneath there is nothing to notice missing.

So underflow tests preload a floor of marked values — `0x5EED0001`,
`0x5EED0002`, … — improbable enough that finding one in a result means a
command reached below its own arguments. A command with declared arity N is
given N-1 sentinels, then must both report the error and leave the floor
untouched.

Verified against a deliberately injected `pop`-then-check bug:

```
empty-stack test:  errors=['? stack underflow']   -> PASSES, bug invisible
sentinel test:     'add' on underflow: sentinel floor disturbed
                     expected: ['S1']
                     found:    []
```

The same scaffolding covers the opposite error. Given exactly the right
arguments with a floor beneath them, a command that pops one more than it
declared shows up as a missing sentinel
(`test_commands_do_not_reach_below_their_arguments`). That bug is invisible
in normal use, because the extra value is usually there — it just quietly
eats the caller's data.

Stack words (`clear`, `dup`, `drop`, `swap`, …) are excluded from floor
checks, since rearranging the stack is their job. Each still needs a
hand-written test asserting the exact rearrangement; keep the list in
`STACK_WORDS` current.

## Macros and multi-line blocks

`cli.macro(name, body)` is a context manager that defines a macro and
forgets it afterwards. Two reasons to use it: at 115200 a long test line is
real wall-clock time and the sweeps send thousands of them, and a named step
reads better than a repeated wall of words. The cleanup is not optional —
a macro that outlives its test makes later tests depend on execution order.

Macros show up in the word list, so a leaked one fails the coverage gate as
an untested command. That is the gate working.

`session.execute_block([...])` frames several lines as one correlated
response: begin marker on the first line, end marker after the last. Use it
for macro definitions that span lines and for setup sequences that should
read as one step. Stack state carries across lines within a block, and an
error on any line is reported rather than masked by the lines around it.

## The coverage gate

`tests/command_coverage.json` lists commands with a real test behind them.
`test_all_commands_are_covered` compares it against the command list the
target reports at runtime, so adding a word to the firmware fails the build
until someone writes a case for it. Without this, a complete test set decays
quietly with every release.

---

## Firmware requirements

The harness assumes four things. R1 and R2 are load-bearing; without them no
amount of harness cleverness recovers correctness.

**R1 — the text-print word emits its text alone on a line.**
Markers are matched with `line.strip() == token`, which is what makes framing
immune to command echo: the echoed command line *contains* the marker but is
not *equal* to it.

**R2 — async output carries a distinguishing prefix.**
An event that fires mid-command must be distinguishable from that command's
result. Without a prefix the two are the same bytes in the same stream.

**R3 — `.s` prints stack depth and every value.**
Values, not just depth, are what make sentinel testing work. Set
`config.stack_format` to match the real output;
`test_stack_parser_agrees_with_target` pushes known values and fails until
it does, so the format is verified rather than assumed.

**R4 — a describe word prints a parseable stack effect.**
Load-bearing in three places: it drives the generated underflow tests, the
conformance sweep, and the derived shuffler assertions. Arity has to be
machine-readable, and `<name>` markers must appear for text-consuming words
or the sweep will invoke them with no text.

### Recommended additions

- **Distinguish printed from pushed results in the help notation.** Today
  `( -- n )` could mean either, so the harness has to accept both depths.
  Something like `( -- ) .n` for printed would let it assert exact depth on
  every command. Small convention change, precise check across the board.
- **A CRC-over-memory-range word.** One command validates the memory read
  path across kilobytes against a host-side CRC of the ELF, and doubles as a
  field image-integrity check.
- **Declare `max_line_length`** to match the real input buffer, so buffer
  overruns are a clear message rather than a timeout.

---

## What building this turned up

Three bugs found by running the harness against a reference target, all of
which would have bitten on real hardware:

**1. A trailing end marker never executes after an error.**
An error aborts the rest of its line, so a marker appended to the command
line is skipped exactly when a negative test needs it. Every negative test
reported a timeout instead of the error the target had correctly sent. Fixed
by sending the end marker as its own line. Bonus: `timed_out` now means the
interpreter is actually stuck, which is a genuinely different diagnosis from
"the command failed".

**2. `^\s*\?\b` matches nothing.**
Both `?` and the following space are non-word characters, so there is no word
boundary between them. Every `? stack underflow` read as success. This is the
most dangerous single line of config in the harness: if `error_pattern` fails
to match, the whole suite reports green while the CLI rejects everything.
`test_error_detection_matches_real_errors` now pins it against a real error.

**3. Reset cost multiplies across the suite.**
The isolation fixture issues `resetcli` twice per test. A 0.2s settle window
that looked harmless in isolation added two minutes to an 83-test run. It is
drain-based now, with the fixed wait removed; raise `reset_settle` only if
hardware genuinely needs longer.

**4. Framing overhead eats the line buffer.**
Roughly 20–30 characters per command. A command near the buffer limit has its
whole line discarded, marker included. The harness now checks length before
transmitting, and treats an end marker arriving without its begin marker as
an instant lost-frame diagnosis rather than a timeout.

## Suggested order of work

1. Fill in `config.py` against the real CLI. Get `test_link.py` green.
2. Confirm the CRC matches the firmware byte for byte, then set
   `crc_enabled=True`. A CRC that disagrees is worse than no CRC.
3. Get `test_reset.py` green. The whole suite's independence rests on
   `resetcli`, so it is worth confirming before anything is built on it.
4. Run `test_stack_display_order` first, then the rest of `test_stack.py`.
   Everything downstream depends on the parser reading `.s` correctly. Every stack assertion
   in the suite depends on that parser being right.
5. Run `test_effect_report`, fix any comments it shows as wrong, then turn
   on `test_stack_effects.py`.
6. Get `test_parser.py` green, including the sentinel underflow sweep. This
   is the trusted-interpreter milestone.
7. Populate `command_coverage.json` and turn on the gate in CI.
8. Move to hardware and enable the timing guard.

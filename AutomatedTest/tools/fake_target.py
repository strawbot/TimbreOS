#!/usr/bin/env python3
"""
A throwaway stand-in for the TimbreOS CLI, speaking the protocol the harness
expects. Not a model of your firmware — it exists so the harness can be
developed, debugged and demonstrated before a board is on the bench, and so
the protocol contract is written down somewhere executable.

Run the suite against it with:
    pytest --transport=host

Point config.host_binary at the real host build to replace it.

Implements the contract the harness depends on:
  R1  the text-print word emits its text alone on a line
  R2  async output carries a distinguishing prefix
  R3  a depth word prints parameter stack depth as a bare integer
  R4  a describe word prints a parseable stack effect
"""

from __future__ import annotations

import random
import sys

STACK: list[int] = []
MAX_DEPTH = 64
TIMING = {
    "motor_update": [1240, 2000, 85312],
    "telemetry_tx": [310, 1000, 9104],
    "cli_service": [95, 500, 41200],
}

MACROS: dict[str, list[str]] = {}
NAPS: list[int] = []
STRINGS: dict[int, str] = {}
_next_addr = [0x1000]

HELP = {
    ".s": "\u002es ( -- ) print stack depth and all values",
    "sp!": "sp! ( ... -- ) empty the parameter stack",
    "echo": "echo ( -- ) <text> print the following word",
    "words": "words ( -- ) list every command name",
    "help": "help ( -- ) <name> describe one command",
    "times": "times ( -- ) longest execution time per action",
    "times.clear": "times.clear ( -- ) zero the recorded maxima",
    "add": "add ( a b -- sum ) add the top two values",
    "peek": "peek ( addr -- value ) read a 32-bit word",
    ":": ": ( -- ) <name> begin a macro definition",
    ";": "; ( -- ) end a macro definition",
    "forget": "forget ( -- ) <name> remove a macro",
    "dup": "dup ( n -- n n ) duplicate the top value",
    "drop": "drop ( n -- ) discard the top value",
    "swap": "swap ( a b -- b a ) exchange the top two",
    "over": "over ( a b -- a b a ) copy the second value to the top",
    "nap": "nap ( ticks -- ) schedule a deferred wake",
    "naps": "naps ( -- ) count pending naps",
    "resetcli": "resetcli ( -- ) reset macros, stacks, naps, parser",
    "strlen": "strlen ( a -- c ) return length of a string",
    "type": "type ( a n -- ) output n characters starting at a",
    "cr": "cr ( -- ) send end of line to output device",
}


def intern_string(text: str) -> int:
    addr = _next_addr[0]
    _next_addr[0] += 1
    STRINGS[addr] = text
    return addr


PROMPT = "ar: "


def emit(text: str) -> None:
    sys.stdout.write(text + "\n")
    sys.stdout.flush()


def maybe_event() -> None:
    """Fire an unsolicited event now and then, to exercise R2."""
    if random.random() < 0.10:
        emit(f"! tick {random.randint(1000, 9999)}")


def run_word(word: str, rest: list[str]) -> bool:
    """Execute one word. Returns False to abort the rest of the line."""
    if word.startswith('"'):
        text = word[1:-1] if len(word) >= 2 and word.endswith('"') else word[1:]
        STACK.append(intern_string(text))
        return True

    if word == "strlen":
        if not STACK:
            emit("? stack underflow")
            return False
        addr = STACK.pop()
        STACK.append(len(STRINGS.get(addr, "")))
        return True

    if word == "type":
        if len(STACK) < 2:
            emit("? stack underflow")
            return False
        count = STACK.pop()
        addr = STACK.pop()
        sys.stdout.write(STRINGS.get(addr, "")[:count])
        sys.stdout.flush()
        return True

    if word == "cr":
        sys.stdout.write("\n")
        sys.stdout.flush()
        return True

    if word == "echo":
        if not rest:
            emit("? echo needs text")
            return False
        emit(rest.pop(0))
        return True

    if word == "resetcli":
        STACK.clear()
        MACROS.clear()
        NAPS.clear()
        emit("cli reset")
        return False          # stops interpreting: rest of line discarded

    if word == "nap":
        if not STACK:
            emit("? stack underflow")
            return False
        NAPS.append(STACK.pop())
        return True

    if word == "naps":
        emit(str(len(NAPS)))
        return True

    if word == "dup":
        if not STACK:
            emit("? stack underflow")
            return False
        STACK.append(STACK[-1])
        return True

    if word == "drop":
        if not STACK:
            emit("? stack underflow")
            return False
        STACK.pop()
        return True

    if word == "swap":
        if len(STACK) < 2:
            emit("? stack underflow")
            return False
        STACK[-1], STACK[-2] = STACK[-2], STACK[-1]
        return True

    if word == "over":
        if len(STACK) < 2:
            emit("? stack underflow")
            return False
        STACK.append(STACK[-2])
        return True

    if word == ".s":
        emit(f"{len(STACK)} stack items: " + " ".join(str(v) for v in STACK))
        return True

    if word == ":":
        if not rest:
            emit("? macro needs a name")
            return False
        name = rest.pop(0)
        body = []
        while rest and rest[0] != ";":
            body.append(rest.pop(0))
        if not rest:
            emit("? unterminated macro")
            return False
        rest.pop(0)
        MACROS[name] = body
        return True

    if word == "forget":
        if not rest:
            emit("? forget needs a name")
            return False
        MACROS.pop(rest.pop(0), None)
        return True

    if word in MACROS:
        expansion = list(MACROS[word])
        while expansion:
            if not run_word(expansion.pop(0), expansion):
                return False
        return True

    if word == "sp!":
        STACK.clear()
        return True

    if word == "words":
        emit(" ".join(sorted(set(HELP) | set(MACROS))))
        return True

    if word == "help":
        if not rest:
            emit("? help needs a name")
            return False
        name = rest.pop(0)
        emit(HELP.get(name, f"? unknown command {name}"))
        return True

    if word == "times":
        emit("ACTION            MAX   LIMIT   RUNS")
        for name, (longest, limit, runs) in TIMING.items():
            emit(f"{name:<16} {longest:>5} {limit:>7} {runs:>6}")
        return True

    if word == "times.clear":
        for row in TIMING.values():
            row[0] = 0
        return True

    if word == "add":
        # Depth is checked BEFORE any pop. A command that pops first and
        # discovers underflow afterwards still reports the error correctly
        # but leaves the stack corrupted, which is exactly what the
        # sentinel tests are built to detect.
        if len(STACK) < 2:
            emit("? stack underflow")
            return False
        b, a = STACK.pop(), STACK.pop()
        STACK.append(a + b)
        return True

    if word == "peek":
        if not STACK:
            emit("? stack underflow")
            return False
        addr = STACK.pop()
        STACK.append(addr ^ 0xA5A5A5A5)
        return True

    # Not a command, so try to read it as a number.
    try:
        value = int(word, 0)
    except ValueError:
        # Matches the real firmware's format (cli.c interpret_word): the
        # word is echoed back followed by "<- eh?", not a leading "?".
        emit(f"{word}<- eh?")
        return False

    if len(STACK) >= MAX_DEPTH:
        emit("? stack overflow")
        return False
    STACK.append(value)
    return True


def tokenize(line: str) -> list[str]:
    """Split on whitespace, except a quoted run counts as one token.

    Mirrors cli.c's postQuote(): a leading `"` reads to the next `"`
    (or end of line) as one token, spaces and all.
    """
    tokens: list[str] = []
    i, n = 0, len(line)
    while i < n:
        while i < n and line[i].isspace():
            i += 1
        if i >= n:
            break
        if line[i] == '"':
            j = line.find('"', i + 1)
            j = n if j == -1 else j + 1
            tokens.append(line[i:j])
            i = j
        else:
            j = i
            while j < n and not line[j].isspace():
                j += 1
            tokens.append(line[i:j])
            i = j
    return tokens


def main() -> None:
    for raw in sys.stdin:
        line = raw.rstrip("\r\n")
        if len(line) > 512:
            emit("? line too long")
            continue
        maybe_event()
        words = tokenize(line)
        while words:
            if not run_word(words.pop(0), words):
                break  # an error aborts the remainder of the line
        maybe_event()


if __name__ == "__main__":
    main()

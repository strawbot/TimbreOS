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

import math
import random
import struct
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
    ".f": ".f ( f -- ) print f as a floating point number",
    "pi": "pi ( -- f ) return 32 bit value for pi",
    "i>f": "i>f ( i -- f ) convert integer to 32 bit float",
    "f>i": "f>i ( f -- i ) convert 32 bit float to integer",
    "fabs": "fabs ( f -- f ) get absolute value of float",
    "fneg": "fneg ( f -- f ) reverse the sign of the float",
    "f+": "f+ ( f m -- f ) add two floats",
    "f-": "f- ( f m -- f ) subtract float m from f",
    "f*": "f* ( f m -- m ) multiply two floats",
    "f/": "f/ ( f m -- m ) divide float f by float m",
    "f>": "f> ( f m -- flag ) flag is true if f is greater",
    "sqrt": "sqrt ( f -- f ) calculate the square root of f",
    "cbrt": "cbrt ( f -- f ) calculate the cube root of f",
    "pow": "pow ( f m -- f ) calculate the power of f to m",
    "exp": "exp ( f -- f ) calculate the value of e to the power of f",
    "ln": "ln ( f -- f ) calculate the natural log of f",
    "log": "log ( f -- f ) calculate the log 10 of f",
    "sin": "sin ( f -- f ) calculate the sine of f",
    "cos": "cos ( f -- f ) calculate the cosine of f",
    "tan": "tan ( f -- f ) calculate the tangent of f",
    "asin": "asin ( f -- f ) calculate the inverse sine of f",
    "acos": "acos ( f -- f ) calculate the inverse cos of f",
    "atan": "atan ( f -- f ) calculate the inverse tan of f",
    "atan2": "atan2 ( f m -- m ) calculate the inverse tan of f/m",
}


def _pack_f32(x: float) -> bytes:
    try:
        return struct.pack("f", x)
    except OverflowError:
        # x is finite but exceeds float32 range (e.g. exp(1000) as a
        # double) - real 32-bit float overflow saturates to +-inf rather
        # than faulting, so that's what gets stored here too.
        return struct.pack("f", math.inf if x > 0 else -math.inf)


def to_f32(x: float) -> float:
    return struct.unpack("f", _pack_f32(x))[0]


def push_float(f: float) -> None:
    STACK.append(struct.unpack("I", _pack_f32(f))[0])


def pop_float() -> float:
    bits = STACK.pop() & 0xFFFFFFFF
    return struct.unpack("f", struct.pack("I", bits))[0]


def safe_math(fn, *args):
    """C's libm returns NaN/inf for domain errors instead of raising, so
    the conformance/sentinel sweeps that call every discovered command
    with garbage values don't crash real hardware - a sentinel bit
    pattern reinterpreted as a float is still a legal (if wild) float32,
    including actual NaN/Inf patterns. Python's math module raises
    instead, which took the whole fake_target.py subprocess down the
    first time a sweep handed sqrt/ln/etc. a negative sentinel. This
    matches the non-crashing behavior instead.
    """
    try:
        result = fn(*args)
    except (ValueError, OverflowError, ZeroDivisionError):
        return float("nan")
    return float("nan") if isinstance(result, complex) else result


def looks_like_float(word: str) -> bool:
    """Same shape cli.c's signDigits() accepts under FLOAT_SUPPORT: digits
    with exactly one decimal point, optionally signed. No exponents -
    Python's float() accepts "1e10"; the real parser doesn't, so that's
    explicitly excluded rather than just delegated to float()'s own rules.
    """
    if "e" in word.lower():
        return False
    if "." not in word:
        return False
    try:
        float(word)
    except ValueError:
        return False
    return True


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

    if word == ".f":
        if not STACK:
            emit("? stack underflow")
            return False
        emit(f"{pop_float():.4f} ")
        return True

    if word == "pi":
        push_float(math.pi)
        return True

    if word == "i>f":
        if not STACK:
            emit("? stack underflow")
            return False
        push_float(to_f32(float(STACK.pop())))
        return True

    if word == "f>i":
        # Rounds, not truncates - matches float.c cliFtoi()'s +-0.5 before
        # the cast (test_f_to_i_rounds_rather_than_truncates pins this).
        if not STACK:
            emit("? stack underflow")
            return False
        f = pop_float()
        if not math.isfinite(f):
            # int(nan) / int(inf) raise in Python; the real (Integer) cast
            # in C is implementation-defined but doesn't fault. 0 is a
            # reasonable non-crashing stand-in, not a claim about what the
            # real board would produce.
            STACK.append(0)
            return True
        f += -0.5 if f < 0.0 else 0.5
        STACK.append(int(f))
        return True

    if word == "fabs":
        if not STACK:
            emit("? stack underflow")
            return False
        push_float(abs(pop_float()))
        return True

    if word == "fneg":
        if not STACK:
            emit("? stack underflow")
            return False
        push_float(-pop_float())
        return True

    if word in ("f+", "f-", "f*", "f/"):
        # ( f m -- ): m is on top, f below - popped in that order.
        if len(STACK) < 2:
            emit("? stack underflow")
            return False
        m = pop_float()
        n = pop_float()
        if word == "f/":
            push_float(safe_math(lambda: n / m))
        else:
            push_float({"f+": n + m, "f-": n - m, "f*": n * m}[word])
        return True

    if word == "f>":
        if len(STACK) < 2:
            emit("? stack underflow")
            return False
        m = pop_float()
        n = pop_float()
        STACK.append(1 if n > m else 0)
        return True

    if word == "sqrt":
        if not STACK:
            emit("? stack underflow")
            return False
        push_float(safe_math(math.sqrt, pop_float()))
        return True

    if word == "cbrt":
        if not STACK:
            emit("? stack underflow")
            return False
        v = pop_float()
        push_float(math.copysign(abs(v) ** (1 / 3), v))
        return True

    if word == "pow":
        if len(STACK) < 2:
            emit("? stack underflow")
            return False
        m = pop_float()
        n = pop_float()
        push_float(safe_math(lambda: n ** m))
        return True

    if word == "exp":
        if not STACK:
            emit("? stack underflow")
            return False
        push_float(safe_math(math.exp, pop_float()))
        return True

    if word == "ln":
        if not STACK:
            emit("? stack underflow")
            return False
        push_float(safe_math(math.log, pop_float()))
        return True

    if word == "log":
        if not STACK:
            emit("? stack underflow")
            return False
        push_float(safe_math(math.log10, pop_float()))
        return True

    if word in ("sin", "cos", "tan", "asin", "acos", "atan"):
        if not STACK:
            emit("? stack underflow")
            return False
        fn = {"sin": math.sin, "cos": math.cos, "tan": math.tan,
              "asin": math.asin, "acos": math.acos, "atan": math.atan}[word]
        push_float(safe_math(fn, pop_float()))
        return True

    if word == "atan2":
        if len(STACK) < 2:
            emit("? stack underflow")
            return False
        m = pop_float()
        n = pop_float()
        push_float(safe_math(math.atan2, n, m))
        return True

    # Not a command, so try to read it as a number.
    try:
        value = int(word, 0)
    except ValueError:
        if looks_like_float(word):
            push_float(to_f32(float(word)))
            return True
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

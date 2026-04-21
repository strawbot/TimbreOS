# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Tests

TimbreOS has no standalone build — it is a source library included directly into target projects. The only standalone build is the Qt-based unit test suite.

**Run all tests:**
```
cd TimbreUnitTests
qmake TimbreUnitTests.pro
make
./tst_timbreunitteststest
```

**Run a single test class** (e.g. printers only):
```
./tst_timbreunitteststest -o -,txt 2>&1 | grep -A20 "testPrinters"
```
Qt Test class names match the `test*.h` filenames: `testbyteq`, `testDictionary`, `testStacks`, `testoperations`, `testmemory`, `testoutput`, `testPrinters`, `testcompiler`, `testparsing`, `testinterpreter`, `testFloat`, `testinputstream`, `testDefines`, `testTimeout`.

The unit test suite uses `printersHost.c` (printf-based) instead of `printers.c` (byte-queue-based). Test files include the `.c` source files directly rather than linking — e.g. `#include "../printers.c"`.

---

## TEA Scheduler API

TEA (Time Event Action) is the core scheduler. All functions below are in `tea.h`. Call `init_tea()` once at startup before using any of them.

### Time units

All time arguments use ticks, not raw milliseconds. Convert with these macros:

```c
usec(n)   // n microseconds in ticks
msec(n)   // n milliseconds in ticks
secs(n)   // n seconds in ticks
mins(n)   // n minutes in ticks
hours(n)  // n hours in ticks
```

Maximum schedulable time is ~49 days. There is no `days()` macro.

---

### `later` — queue an action for immediate cooperative execution

```c
void later(void (*action)());
```

Appends `action` to the run queue. It will execute the next time `run()` or `action_slice()` drains the queue. Safe to call from interrupt context via the `safe()` macro.

```c
void blink() { toggle_led(); }

later(blink);   // blink runs on the next scheduler pass
```

`later(no_action)` is silently ignored. `later(NULL)` is a fatal error (`LATER_NULL`). Overflowing `NUM_ACTIONS` is a fatal error (`ACTION_OVERRUN`).

---

### `after` — schedule an action after a delay (non-blocking)

```c
void after(Long t, vector action);
```

Schedules `action` to be enqueued via `later()` after `t` ticks. Does not block. Re-scheduling the same action before it fires moves its due date (the pending entry is replaced, not duplicated).

```c
after(secs(5), sendHeartbeat);      // fires once, 5 seconds from now
after(msec(500), pollSensor);       // fires once, 500 ms from now
```

Calling `after` with `t <= 0` falls through immediately to `later(action)`.

---

### `in` — schedule an action to run asap at a future time

```c
void in(Long t, vector action);
```

Like `after`, but the action runs directly (not via `later`) when its time arrives — it bypasses the cooperative queue and executes in the timer ISR context. Use for timing-critical responses. `t == 0` is a fatal error (`IN_0`); use `later(action)` for zero-delay dispatch.

```c
in(usec(200), triggerPulse);   // runs ~200 µs from now, outside the action queue
```

---

### `now` — run an action immediately and track its stats

```c
#define now(action) actionRun(action)
```

Calls `action` synchronously and records its maximum execution time in the `machineStats` table. Equivalent to calling the function directly, but participates in profiling.

```c
now(processPacket);
```

---

### `Event` / `when` / `never` — single-slot event signals

`Event` is a `vector[1]` — a one-element array of function pointers.

```c
// Declaration (typically in a header)
Event dataReady;

// Register a handler
when(dataReady, processData);

// Fire the event (call its handler)
(*dataReady)();

// Clear the handler
never(dataReady);
```

Only one handler per event at a time — `when` overwrites any prior handler. `never` replaces the handler with `no_action`. A common pattern is for the handler itself to call `never` to act as a one-shot:

```c
void onDataReady() {
    never(dataReady);
    processData();
}
when(dataReady, onDataReady);
```

**C++ `Timebox` macro** — arms both a timeout and an event; whichever fires first cancels the other:

```c
Timebox(secs(3), responseEvent, handleTimeout);
// If responseEvent fires within 3s, handleTimeout is cancelled.
// If 3s elapse first, handleTimeout runs and responseEvent is cleared.
```

---

### `namedAction` — register a function for profiling

```c
#define namedAction(fn) actor(fn, #fn)
void actor(vector action, const char * name);
```

Associates a name string with a function pointer so it appears in `machineStats` output. Call once during initialization for each action you want to profile. The stringification in the macro uses the C identifier name; use `actor(fn, "custom name")` for a different label.

```c
void init() {
    namedAction(pollSensor);
    namedAction(sendHeartbeat);
    namedAction(processPacket);
    init_tea();
}
```

---

### Cancellation

```c
void stop_te(vector v);      // cancel a pending after/in for v
void stop_action(vector v);  // remove v from the action queue
void stop(vector v);         // cancel both
```

---

### Recurring actions

TEA has no built-in periodic timer. The idiomatic pattern is self-rescheduling:

```c
void pollSensor() {
    readSensor();
    after(msec(100), pollSensor);  // reschedule self
}

// Start the loop:
later(pollSensor);
```

---

## Architecture Overview

TimbreOS has three interlocking subsystems:

**TEA** (`tea.h`, `tea.c`) — cooperative scheduler as described above. The main loop calls `run()` continuously; hardware timer interrupts call `check_dueDates()` via `alarmEvent`.

**CLI** (`cli.h`, `cli.c`) — Forth-like stack interpreter with dual stacks (data and return), compiled word definitions, and a text input buffer (TIB). Word lookup searches the linked-list wordlist (user-defined words via `:`), then the compiled `wordnames[]`/`immediatenames[]`/`constantnames[]` arrays generated by `parsewords.py`. Number parsing handles decimal, hex (`0x`), octal (`0c`), binary (`0b`), and floats (a decimal point triggers float parsing via `signDigits`).

**Word List generation** (`WordLists/`, `parsewords.py`) — CLI words are declared in `.txt` files and compiled into C arrays by `parsewords.py` as a pre-build step. Output files (`wordlist.c`, `help.c`) must be regenerated whenever `.txt` files change. `clibindings.txt` is the standard word set; `floatwords.txt` adds float stack operations.

### Supporting modules

- **`printers.c`** — Embedded output through `emitq` (byte queue). `dotnb(field, digits, n, radix)` is the core formatter. `printFloat0(f, n)` prints `n` decimal places; `printFloat(f, n)` adds a trailing space. Use `printersHost.c` as a drop-in replacement for host/test builds.
- **`queue.c` / `byteq.c`** — Circular queues. `QUEUE(size, name)` / `BQUEUE(size, name)` declare and statically initialize queues. Indices count down from `QEND` to `QDATA`, wrapping at `QDATA`.
- **`dictionary.c`** — Hash table for action-name/stats mapping. `HASHDICT(capacity, name)` declares a static dictionary. Supports string keys (`dictInsert`/`dictFind`) and pointer keys (`dictAddKey`/`dictFindKey`).
- **`timeout.c`** — Polling timeouts. `NEW_TO(name)` declares a `Timeout[1]` (no `&` needed). `timeoutWait(t)` yields via `action_slice()`; `ms_delay(t)` is a bare busy-wait for hardware timing and blocks the scheduler.
- **`ttypes.h`** — Portable types: `Cell` = `uintptr_t`, `Long` = `unsigned long`, `Octet` = `unsigned long long`. Use these instead of `int`/`long` in TEA/CLI code.

## Per-Project Configuration

Every project must provide a `project_defs.h` (included by `tea.h` and `timeout.h`). Required symbols:

| Symbol | Purpose |
|---|---|
| `NUM_TE` | Number of time-event slots |
| `NUM_ACTIONS` | Action queue depth |
| `TEA_TABLE` | Hash size for named-action dict (`HASH8`–`HASH16`) |
| `ONE_SECOND` | Timer ticks per second (used by `msec()`, `secs()` macros) |
| `safe(code)` | Atomic section wrapper |
| `ENTER_SAFE_REGION` / `LEAVE_SAFE_REGION` | Interrupt enable/disable |
| `get_ticks()` | Raw hardware tick counter |
| `sysTicks()` | High-resolution tick for execution timing |
| `set_delta_alarm(delta)` | Set next timer interrupt |
| `over_due()` | Called when a time event fires late |

See `example_project_defs.h` for a template.

## Key Invariants

- `in(0, action)` is a fatal error (`BLACK_HOLE(IN_0)`). Use `later(action)` for zero-delay dispatch.
- `safe()` does not work with comma-separated statements; use semicolons inside the block.
- Floats passed as CLI literals are 32-bit `float`. Values like `8.95` are stored as `~8.9499998` (IEEE 754) and will round accordingly — this is correct behavior, not a bug.
- `dotnb(field, digits, n, radix)` with `field == digits != 0` produces exactly `field` digits. The caller must ensure the value fits; there is no truncation guard.
- Re-scheduling the same action with `after`/`in` before it fires moves its due date rather than creating a duplicate entry.

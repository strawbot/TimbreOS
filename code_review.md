# TimbreOS Code Review

**Reviewed:** April 3, 2026
**Scope:** Core library — `tea.c/h`, `printers.c/h`, `dictionary.c/h`, `queue.c/h`, `timeout.c`, `ttypes.h`, `cli.h`

---

## Summary

TimbreOS is a well-structured, mature embedded real-time framework. The architecture is clean, the TEA scheduler design is solid, and the unit test coverage signals a professional development process. The issues below are a mix of latent bugs, portability hazards, and code hygiene items. They are ordered by severity.

---

## Bugs

### 1. `printFloat0` — Fractional carry overflow silently dropped (`printers.c`)

The `.9` → `.5` rounding fix (already applied) is correct, but a second problem remains. When rounding pushes the scaled fractional part to exactly the next power of ten, `dotnb` silently truncates it.

**Example:** `printFloat0(1.999f, 2)`
- Integer part prints as `1`
- Fractional part: `0.999 × 100 + 0.5 = 100.4` → cast to `Cell` = 100
- `dotnb(2, 2, 100, 10)` converts exactly 2 digits: `100 % 10 = 0`, `10 % 10 = 0` → prints `.00`
- Output: `1.00` — should be `2.00`

The fix requires detecting when the rounded fractional result equals the multiplier and carrying 1 into the integer part.

---

### 2. `actor()` — Duplicate name detection never fires (`tea.c`, lines 335–345)

```c
void actor(vector action, const char * name) {
    Cell key = 0xFFFFFFFC & ((Cell)action + 1);
    if (dictFindKey(key, &teanames) == 0) {
        dictAddKey(key, &teatimes);
        dictAddKey(key, &teanames);
        *dictAdjunctKey(key, &teanames) = (Cell)name;
    } else if (cname[0] != 0) {   // ← condition is inverted
        strncpy(cname, name, 31);
        later(bad_name);
    }
}
```

`cname` is a global initialized to all zeros. The `else` branch (reached when a duplicate key is found) only copies the name and schedules `bad_name` when `cname[0] != 0`. Since `cname` starts as zero and is only set to non-zero *inside* this block, the first duplicate is always silently ignored. The condition should be `cname[0] == 0` (copy name and schedule report only when no pending report exists).

---

### 3. `upsizeDict()` — `free` count not decremented after copy (`dictionary.c`, lines 218–250)

`initDict` is called to create a new dictionary, which sets `dict->free = dict->capacity / 2`. Entries from the old dictionary are then copied directly by assigning `dict->table[last]` and `dict->adjunct[last]` without calling `plusEntry()`. After upsizing, `dict->free` reports `capacity/2` regardless of how many entries were transferred, making the "dictionary is full" guard unreliable for the lifetime of the resized dictionary.

---

### 4. `upsizeDict()` — `upsize` flag silently reset (`dictionary.c`)

`upsizeDict` calls `initDict(dict, old.capacity)`, which in turn calls `emptyDict`, which unconditionally sets `dict->upsize = false`. A dictionary configured to upsize on overflow will lose that capability after its first resize and will instead call `emptyDict` (discarding all entries) on the next overflow. `upsizeDict` should preserve the flag.

---

### 5. `actor()` / `printActionName()` — 32-bit key mask on 64-bit `Cell` (`tea.c`, lines 336, 348)

```c
Cell key = 0xFFFFFFFC & ((Cell)action + 1);
```

`0xFFFFFFFC` is a 32-bit literal. On 64-bit platforms, this zeroes the upper 32 bits of `key`, making all action addresses in the upper half of the address space collide. The mask should be written portably as `~(Cell)3`.

---

### 6. `pdump()` — Pointer truncated to 32 bits on 64-bit hosts (`printers.c`, line 160)

```c
printHex((unsigned int)(long)a);
```

`printHex` takes an `unsigned int`. The cast chain loses the upper 32 bits of `a` on any LP64 or LLP64 platform. Should use `(uintptr_t)` or `(unsigned long)` and a matching print function that handles the platform-native pointer width.

---

## Code Quality

### 7. `tabTo()` — Stack-allocated VLA unsafe on embedded targets (`printers.c`, lines 17–26)

```c
char spaces[len + 1];
```

Variable-length arrays are stack-allocated with no overflow protection. If `n` is large or the caller passes an unreasonable tab stop, this will silently corrupt the stack. On microcontrollers with small stacks this is particularly dangerous. A fixed-size scratch buffer with a guarded loop, or repeated single-character output, is safer.

Additionally, the `else` branch when `len <= 0` prints a single space rather than the described backspace behavior (the comment says "use backspaces"). This looks like an unfinished fallback.

---

### 8. `ms_delay()` — Busy-waits without yielding (`timeout.c`, lines 67–72)

```c
void ms_delay(Cell ms) {
    NEW_TO(timer);
    setTimeout(ms, timer);
    while (!checkTimeout(timer));
}
```

Unlike `timeoutWait()`, `ms_delay` spins without calling `action_slice()`. This blocks the entire TEA scheduler for the duration of the delay, starving all pending actions and potentially causing latency spikes or watchdog timeouts. If blocking is intentional (e.g., hardware timing), this should be clearly documented; otherwise `action_slice()` should be added to the loop body as in `timeoutWait`.

---

### 9. `checkTimeout()` — Misleading comment (`timeout.c`, line 13)

```c
if (timer->off == true) // see if it is enabled
    return true; // a timer off is considered done
```

The comment says "see if it is enabled" but the condition tests whether the timer is *disabled* (`off == true`). The comment should read "see if it is disabled."

---

### 10. `outside()` macro — Unsafe comma-expression form (`tea.h`, lines 41–43)

```c
#define outside(action) \
    pushq((Cell)action, outsideq), \
    critical_action()
```

This expands to a comma expression. If used as the sole body of an `if` statement without braces, `critical_action()` will execute unconditionally in all branches. The standard idiom for multi-statement macros is `do { ... } while(0)`.

---

### 11. `play_events()` — Dead assignment in else branch (`tea.c`, line 555)

```c
} else {
    last = t;   // t == last already; this is always a no-op
    print("  ");
}
```

The `else` branch is reached only when `t == last`, so `last = t` changes nothing. This is harmless but suggests the logic may have been modified and the assignment left behind. It can be removed.

---

### 12. `setUpsize()` not declared in `dictionary.h` (`dictionary.c`, line 192)

`setUpsize(bool flag, dictionary_t * dict)` is implemented but not listed in the header. Callers must either declare it themselves or include the `.c` file. It should be added to `dictionary.h`.

---

### 13. `te_lists()` — NULL pointer used in pointer arithmetic (`tea.c`, line 46)

```c
print("\ntodo, done: "), printDec(te_todo.next - tes), printDec(te_done.next - tes);
```

If `te_todo.next` or `te_done.next` is `NULL` (a valid empty-list state), pointer subtraction with `tes` produces undefined behavior. The function is a debug/diagnostic tool so the impact is low, but it should guard against NULL before computing the offset.

---

### 14. `TeaCup` — `cups` constant not reflected in `OMVI` macro (`tea.h`, lines 137–170)

`static const Byte cups = 3` and the `OMVI` macro hardcodes three lambda slots (`obj[0]`, `obj[1]`, `obj[2]`). Increasing `cups` will not automatically expand the macro. This tight coupling is a maintenance hazard; consider generating the lambda array or using a different dispatch mechanism if `cups` is expected to change.

---

## Open Issues from `log.txt`

Several items in the project log are worth tracking for future work:

- **Issue 17** — Stack underflow detection not yet implemented in the CLI. An underflow on the data or return stack currently produces silent data corruption.
- **Issue 9** — No length protection in `setPrompt`; an excessively long prompt string could overflow internal buffers.
- **Issue 7** — Time values use `Long` throughout. A dedicated `Timestamp` typedef would clarify intent and make it easier to change the underlying type if the 49-day rollover limit becomes a constraint.
- **Issue 12** — Byte queue and cell queue have parallel but divergent implementations. Unifying them (ideally under a C++ template or macro-generated code) would reduce duplication and maintenance surface.

---

## Positive Observations

- The TEA scheduler's separation of time events, actions, and signals is architecturally clean and avoids common pitfalls of bare-metal scheduling.
- The `safe()` macro pattern for atomic sections is consistently applied in interrupt-sensitive paths.
- The latency measurement infrastructure (`measure_latency`, `machineStats`) is a valuable built-in tool rarely found in embedded frameworks at this level.
- The dictionary implementation correctly handles collisions, ordering, and deletion tombstones (`zeroString`), which is non-trivial.
- The unit test suite in `TimbreUnitTests/` reflects genuine investment in correctness.

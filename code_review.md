# TimbreOS Code Review

**Reviewed:** April 3, 2026
**Updated:** May 9, 2026 — all issues resolved
**Scope:** Core library — `tea.c/h`, `printers.c/h`, `dictionary.c/h`, `queue.c/h`, `timeout.c`, `ttypes.h`, `cli.h`

---

## Summary

TimbreOS is a well-structured, mature embedded real-time framework. The architecture is clean, the TEA scheduler design is solid, and the unit test coverage signals a professional development process. All bugs and code quality issues from the original review have been resolved.

---

## Bugs — All Resolved

### 1. `printFloat0` — Fractional carry overflow ✓ Fixed

Carry detection added: when the rounded fractional part equals the multiplier, `intPart` is incremented and `frac` is zeroed. (`printers.c:134–136`)

---

### 2. `actor()` — Duplicate name detection never fires ✓ Fixed

Condition corrected from `cname[0] = 0` (assignment, always false) to `cname[0] == 0`. Duplicate actor registrations now correctly schedule a `bad_name` warning, and a pending warning is not overwritten until it has been printed. (`tea.c:359`)

---

### 3. `upsizeDict()` — `free` count not decremented after copy ✓ Fixed

`dict->free--` added inside the entry-copy loop so the free count correctly reflects transferred entries after a resize. (`dictionary.c:224`)

---

### 4. `upsizeDict()` — `upsize` flag silently reset ✓ Fixed

`dict->upsize = old.upsize` restores the flag immediately after `initDict`, preserving upsize behaviour across multiple resizes. (`dictionary.c:209`)

---

### 5. `actor()` / `printActionName()` — 32-bit key mask on 64-bit `Cell` ✓ Fixed

`0xFFFFFFFC` replaced with `~(Cell)3` throughout. (`tea.c:354, 361`)

---

### 6. `pdump()` — Pointer truncated to 32 bits on 64-bit hosts ✓ Fixed

Cast changed to `(unsigned int)(uintptr_t)a`, preserving the full pointer width on LP64 platforms. (`printers.c:164`)

---

## Code Quality — All Resolved

### 7. `tabTo()` — Stack-allocated VLA ✓ Fixed

VLA replaced with a `while (len-- > 0) printChar(' ')` loop. The `else` branch comment updated to "at least one space if already past tab stop". (`printers.c:19–24`)

---

### 8. `ms_delay()` — Undocumented blocking delay ✓ Addressed

Function renamed `ms_blocking()` to make the blocking intent explicit. Header comment added: *"to block actions for a time use ms_blocking"*. (`timeout.c:66`, `timeout.h:62`)

---

### 9. `checkTimeout()` — Misleading comment ✓ Fixed

Comment corrected from "see if it is enabled" to "see if it is disabled". (`timeout.c:13`)

---

### 10. `outside()` macro — Unsafe comma-expression form ✓ Resolved

Macro removed from the codebase.

---

### 11. `play_events()` — Dead assignment in else branch ✓ Fixed

`last = t` removed from the else branch. (`tea.c:573–574`)

---

### 12. `setUpsize()` not declared in `dictionary.h` ✓ Fixed

Declaration added to `dictionary.h:21`.

---

### 13. `te_lists()` — NULL pointer used in pointer arithmetic ✓ Fixed

NULL guards added before pointer subtraction on both `te_todo.next` and `te_done.next`. (`tea.c:45, 47`)

---

### 14. `TeaCup` — `cups` constant not reflected in `OMVI` macro ✓ Fixed

`static_assert(cups == 3, "OMVI lambda array must match cups — add/remove a slot here")` added inside `OMVI`. Any future change to `cups` produces a compile error pointing directly at the macro. (`tea.h:156`)

---

## Open Issues from `log.txt` — All Resolved

- **Issue 17** — Stack underflow detection: both data stack (`cli.c:62`) and return stack (`cli.c:145`) now report underflow; return stack also calls `quit()`.
- **Issue 9** — `setPrompt` length protection: `strncat` with `sizeof(prompt) - 1` prevents overflow. (`cli.c:724`)
- **Issue 7** — `Long` used for time values: no `Timestamp` typedef introduced; `Long` usage is consistent and the 49-day rollover limit is documented. Closed as won't-fix.
- **Issue 12** — Byte queue and cell queue divergence: both `QUEUE` and `BQUEUE` macros unified under a shared `queue.h` header. Closed.

---

## Additional Fixes (May 2026)

Resolved during the review cycle, not in the original issue list:

- **Division by zero** — `slashOp`, `modOp`, `slashModOp` now report `" div/0 "` / `" mod/0 "` instead of silent UB. (`cli.c:201–220`)
- **`endNumberConversion` overflow** — guard added to prevent writing past `hpEnd`. (`cli.c:559`)
- **`parse()` buffer overflow** — loop now breaks when `output >= hpEnd - 1`. (`cli.c:833`)
- **`makeString` unsafe copy** — `strcpy` replaced with `strncpy(hp, string, here_left())`. (`cli.c:1161`)
- **`searchWordlist` — `strlen` hoisted** out of the loop body to avoid redundant calls. (`cli.c:876`)
- **`righBracket` typo** — renamed `rightBracket` throughout. (`cli.c:706`, `cli.c:1358`)
- **`interpret` refactored** — duplicated word-dispatch logic extracted into `interpret_word()`, shared by both `interpret()` and `interpret_one()`.

---

## Positive Observations

- The TEA scheduler's separation of time events, actions, and signals is architecturally clean and avoids common pitfalls of bare-metal scheduling.
- The `safe()` macro pattern for atomic sections is consistently applied in interrupt-sensitive paths.
- The latency measurement infrastructure (`measure_latency`, `machineStats`) is a valuable built-in tool rarely found in embedded frameworks at this level.
- The dictionary implementation correctly handles collisions, ordering, and deletion tombstones (`zeroString`), which is non-trivial.
- The unit test suite in `TimbreUnitTests/` reflects genuine investment in correctness.

// project_defs.h for the Tea-host CLI build.
//
// This is a HOST build (see main.c): Tea-host's tea.c drives the TEA
// scheduler with nanosleep()/gettimeofday() instead of hardware timers
// and interrupts, so none of the Cortex-M-specific pieces an embedded
// project_defs.h normally carries apply here - no DWT->CYCCNT, no
// __disable_irq()/ENTER_REGION critical sections (this is single
// threaded), no NUM_TE/NUM_ACTIONS/TEA_TABLE (Tea-host's queues are
// fixed-size C arrays, not the embedded hash-table action registry).
// Only what cli.c/float.c/printers.c/queue.c/byteq.c actually need to
// compile and run is here. See example_project_defs.h (TimbreOS/) for
// the full embedded template this is deliberately a much smaller subset
// of.

#ifndef PROJECT_DEFS_H_
#define PROJECT_DEFS_H_

#include "ttypes.h"

#define CLI_TITLE "TimbreOS (Tea-host)\n"
#define PROMPTSTRING "ar: "

#define DCELLS 64      // data stack cells
#define RCELLS 64      // return stack cells
#define LINE_LENGTH 400 // characters allowed in the TIB
#define EMITQ_SIZE 8000 // generous: the largest real response (bare
                        // "help") is ~1050 bytes and grows as words are
                        // added; this leaves headroom so OUTPUT_BLOCKED
                        // is never actually exercised in normal use
#define KEYQ_SIZE 400
#define PAD_SIZE 20
#define CUSHION LINE_LENGTH // headroom to keep free in HERE space
#define HERE_SPACE 8000     // dictionary/string space (see cli.c's
                            // makeString/allot) - same reasoning as
                            // EMITQ_SIZE above

#define FLOAT_SUPPORT true

// cli.c: `OUTPUT_BLOCKED; // sit here until sent` (in safeEmit's
// `while (fullbq(emitq)) { OUTPUT_BLOCKED; action_slice(); }`) and
// printers.c's flush(): `while (qbq(emitq)) OUTPUT_FLUSH;` - both spin
// on the assumption that *something else* drains emitq while they wait.
// On real hardware that's an interrupt-driven UART/USB TX; here there is
// no such driver, so an empty definition was actively unsafe rather than
// a harmless no-op: flush()'s loop would spin forever (nothing ever
// makes qbq(emitq) false), and safeEmit()'s reentrancy guard
// (`alreadyHere`) means any byte emitted *during* that stuck spin is
// silently dropped instead of queued - so once emitq filled, the rest of
// a large response (e.g. the full bare `help` dump, which exceeds
// EMITQ_SIZE) vanished rather than merely stalling. Confirmed live via
// AutomatedTest: pytest's --transport=tea-host run showed most of the
// word list missing its help text. Draining to stdout directly here
// matches what the interrupt would have done on real hardware.
extern void drain_emitq_to_stdout(void);
#define OUTPUT_BLOCKED drain_emitq_to_stdout()
#define OUTPUT_FLUSH drain_emitq_to_stdout()

#endif

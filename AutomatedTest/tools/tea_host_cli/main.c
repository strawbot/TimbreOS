// Tea-host CLI: runs the real TimbreOS CLI (cli.c/float.c/printers.c) on
// the host, using Tea-host's TEA scheduler port instead of a hand-written
// Python reimplementation (tools/fake_target.py) - so --transport=host
// can exercise the actual firmware source, not a parallel copy of it
// that drifts out of sync with real changes.
//
// Protocol: feed stdin into the CLI's own key queue via keyIn() - the
// same entry point real hardware's UART RX ISR uses - then drive the TEA
// scheduler until it goes idle. No special-casing of the framing
// protocol here; this binary just IS the CLI, reading and writing
// exactly what a real board would over serial.
//
// Reading: a whole available chunk at a time via read(), not stdio's
// getchar() one byte per syscall. This matters for a reason specific to
// cli.c, not just performance: interpreting()'s "is there more to do"
// check includes peek() (tib.buffer[tib.in]), and the TIB is only ever
// null-terminated at the position a line ended at - not at every later
// position a shorter line touches - so peek() can read a stale non-zero
// byte left over from a longer previous line and report "more to
// interpret" when there isn't. On real hardware this is invisible: UART
// RX is interrupt-driven independently of the main loop's run()/cli()
// chain, so by the time that stale check is made, more bytes have
// usually already arrived and drained into keyq, overwriting the stale
// position, for the entire time such a "spin" lasts. Feeding one byte
// from getchar() at a time and driving the scheduler to idle before
// reading the next reproduces the opposite of that: nothing new can
// arrive while a single byte's worth of driving is stuck spinning on a
// stale peek(), because the only thing that could feed it more is the
// very call this is blocking. Confirmed live: it either hung (spun
// indefinitely on a byte no new input could resolve) or, with the spin
// bounded, went out of sync (moved on with the CLI mid-word, spilling
// one command's terminal echo into the next command's response window).
// Reading whatever the pipe already has queued, all at once, and *then*
// draining to idle matches the interrupt-fed case instead: every byte
// that could resolve a stale peek() is already in keyq before the drain
// starts.

#include "tea.h"
#include "cli.h"
#include "printers.h"
#include "byteq.h"
#include <stdio.h>
#include <unistd.h>

void init_cli(void);
void keyIn(Byte c);
void action_slice(void);
bool pending_actions(void);

// help.c's word-filtering hook - every board provides one (see
// board-NanoR4/Board/board_cli.c), currently always "true" there too.
// Nothing host-specific to filter, so this matches that exactly.
bool visible_word(char *s) { (void)s; return true; }

// cli.c's safeEmit() pushes every output byte into emitq (BQUEUE(EMITQ_SIZE,
// emitq), declared non-static in cli.c - byteq.h's own comment says
// "declare instances as extern Cell name[]"). On real hardware a UART/USB
// TX interrupt drains it; there's no such driver in a host build, so this
// does the equivalent by hand: pull whatever's queued and write it to
// stdout. Called after every processed input chunk below, and also wired
// up as OUTPUT_BLOCKED/OUTPUT_FLUSH (project_defs.h) so cli.c/printers.c's
// own spin-until-drained loops actually make progress instead of either
// hanging (flush()) or silently dropping bytes once emitq fills
// (safeEmit()'s reentrancy guard) - see project_defs.h for the full story.
extern Cell emitq[];
extern Cell keyq[];

void drain_emitq_to_stdout(void) {
    while (qbq(emitq))
        putchar(pullbq(emitq));
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    init_tea();
    init_cli();
    drain_emitq_to_stdout(); // the startup banner + first prompt

    Byte buf[4096];
    ssize_t n;
    while ((n = read(0, buf, sizeof buf)) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            // keyIn() drops a byte (and condemns the line) when the key
            // queue is full. A whole read() chunk can be bigger than
            // KEYQ_SIZE, and unlike hardware nothing drains keyq while the
            // bytes are still being fed in - so make room the way the main
            // loop would have, by running the scheduler when it fills.
            for (int g = 0; g < 100000 && fullbq(keyq); g++)
                action_slice();
            keyIn(buf[i]);
        }
        // Bounded, not unconditional "while (pending_actions())": that's
        // still correct backstop against a genuine stuck chain (as
        // opposed to a merely-stale-until-more-input one), same as
        // before.
        for (int i = 0; i < 100000 && pending_actions(); i++)
            action_slice();
        drain_emitq_to_stdout();
    }
    return 0;
}

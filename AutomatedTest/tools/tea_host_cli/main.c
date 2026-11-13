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

#define _GNU_SOURCE // posix_openpt() & friends under -std=c11 on glibc; harmless on macOS
#include "tea.h"
#include "cli.h"
#include "printers.h"
#include "byteq.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
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

// Where output goes: stdout normally, the pty master under --pty.
static int out_fd = 1;
// Under --pty, end lines the way a serial terminal expects: CR LF. (The tty's
// own ONLCR can't do it: output processing applies to what the slave side
// writes, and the CLI's output enters through the master.)
static bool crlf = false;

static void write_all(const char *p, size_t n) {
    while (n) {
        ssize_t w = write(out_fd, p, n);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            return;
        }
        p += w;
        n -= (size_t)w;
    }
}

void drain_emitq_to_stdout(void) {
    char out[256];
    size_t k = 0;
    static char prev = 0;
    while (qbq(emitq)) {
        char c = (char)pullbq(emitq);
        if (k + 2 > sizeof out) {
            write_all(out, k);
            k = 0;
        }
        if (crlf && c == '\n' && prev != '\r')
            out[k++] = '\r';
        out[k++] = c;
        prev = c;
    }
    if (k)
        write_all(out, k);
}

// --pty: a serial-style connection. Opens a pseudo-terminal and serves the
// CLI on it, so anything that opens a serial port - screen, minicom, the
// harness's own SerialTransport - can attach to the host build the way it
// attaches to a board. Over plain stdin/stdout this is unusable by hand: the
// tty echoes locally on top of the CLI's own echo, and Enter sends a newline,
// which the CLI ignores (it commits a line on carriage return only).
static int open_pty(void) {
    int m = posix_openpt(O_RDWR | O_NOCTTY);
    if (m < 0 || grantpt(m) < 0 || unlockpt(m) < 0) {
        perror("tea_host_cli: pty");
        exit(1);
    }
    const char *name = ptsname(m);
    if (!name) {
        perror("tea_host_cli: ptsname");
        exit(1);
    }
    // Deliberately NOT kept open: the harness's SerialTransport refuses a port
    // some other process already holds, and a slave fd of ours would make this
    // binary that process. The cost is that the master reads EOF/EIO whenever
    // no client is attached, which the main loop waits out (see main()).
    crlf = true;
    printf("tea_host_cli: serial-style connection on %s\n", name);
    fflush(stdout);
    return m;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    int in_fd = 0;
    if (argc > 1 && strcmp(argv[1], "--pty") == 0)
        in_fd = out_fd = open_pty();
    else if (argc > 1) {
        fprintf(stderr, "usage: %s [--pty]\n", argv[0]);
        return 2;
    }

    init_tea();
    init_cli();
    if (in_fd == 0)
        drain_emitq_to_stdout(); // the startup banner + first prompt
    else
        // Never write to the pty before a client has attached and put it in
        // raw mode. Until then its line discipline has echo on (and macOS
        // resets it whenever the last client closes), so unsolicited output
        // would be echoed straight back as input and the CLI would try to
        // run its own banner. Output only ever follows input from here on,
        // by which point the client has configured the port. The banner is
        // dropped, as a board's is for anyone who wasn't watching at boot.
        while (qbq(emitq))
            pullbq(emitq);

    Byte buf[4096];
    ssize_t n;
    while ((n = read(in_fd, buf, sizeof buf)) > 0 || n <= 0) {
        if (n <= 0) {
            // stdin: EOF is the end of the input. pty master: EOF/EIO just
            // means no client is attached right now (before the first one,
            // or between two), so wait for the next.
            if (in_fd == 0 && n == 0)
                break;
            if (n < 0 && errno != EINTR && errno != EIO)
                break;
            if (in_fd != 0)
                usleep(20000);
            continue;
        }
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

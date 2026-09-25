// printf based printers  Robert Chapman III  Jun 27, 2012

#include <stdio.h>
#include "printers.h"
#include "cli.h"
#include <string.h>
#include <math.h>

#define ESC 0x1B
#define CR 0x0D
#define LF 0x0A

// Every function below used to write straight to stdout via printf(),
// bypassing emitByte()/emitq entirely - unlike printers.c, where print()
// is msg() and everything funnels through the same byte queue. That
// divergence is invisible in isolation (this file was written for
// printf-convenience, not for interleaving with anything else), but it
// broke the moment this file was driven from the real cli.c (Tea-host,
// AutomatedTest/tools/tea_host_cli/Makefile): cli.c's own output (marker
// text, command echo) goes through emitq and is drained by the host
// driver's main loop on its own schedule, while this file's printf()
// calls landed on stdout immediately, inline, mid-command. Confirmed
// live: a bare `help` command's word list appeared on the wire before
// the framing marker that was supposed to precede it, because print()
// raced ahead of the marker's own (properly emitq-queued) output -
// which silently discarded nearly the entire response on the harness
// side, since anything arriving before the begin marker is treated as
// pre-response noise. Routing through msg()/emitByte() - the same path
// printers.c uses - puts this file's output back on the one ordered
// pipeline the framing protocol depends on. getCursor() (cli.c's own
// cursor tracker, updated by emitByte()) replaces the local `outp`
// static this function used to keep in parallel and out of sync with it.

void printCr(void)
{
    print("\n");
}

void tabTo(int n)
{
    int len = n - getCursor();
    if (len > 0)
        while (len-- > 0) printChar(' ');
    else
        printChar(' ');
}

void print(const char *message)
{
    msg((char *)message);
}

char * numString(Byte field, Byte digits, Cell n, Byte radix)
{
/* field digits radix format
 *   0     0      10   "%d"
 *   0     n      10   "%.nd"
 *   w     0      10   "%wd"
 *   w     n      10   "%0w.nd"
 *                     "%0*.*d",w,n
 */
    char * format;

    if (radix == 16)
        format = "%0*.*X";
    else
        format = "%0*.*d";

    static char buf[20];

    snprintf(buf, sizeof(buf), format, field, digits, (unsigned int)n); // Note Host version adds null terminator

    return buf;
}

void dotnb(Byte field, Byte digits, Cell n, Byte radix)
{
    char * format;
    char buf[20];

    if (radix == 16)
        format = "%0*.*X";
    else
        format = "%0*.*d";

    snprintf(buf, sizeof(buf), format, field, digits, (unsigned int)n);
    print(buf);
}

void printHex(unsigned int hex)
{
    char buf[12];
    snprintf(buf, sizeof(buf), "%08X ", hex);
    print(buf);
}

void printDec(unsigned int dec)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d ", (int)dec);
    print(buf);
}

void printDec0(unsigned int dec)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", (int)dec);
    print(buf);
}

void printnDec(unsigned int n, unsigned int dec) { dotnb(n, n, dec, 10); }

void printBin0(unsigned int bin)
{
    int i;

    for(i=0x80; i != 0; i>>=1)
        printChar((((bin)&i) != 0) ? '1' : '0');
}

void printBin(unsigned int bin)
{
    printBin0(bin);
    printChar(' ');
}

void printChar(unsigned char ch)
{
    emitByte(ch);
}

void printHex2(unsigned int hex)
{
    char buf[6];
    snprintf(buf, sizeof(buf), " %02X", hex);
    print(buf);
}

void printFloat(float f, int n)
{
    // Mirrors printers.c's printFloat0 exactly, including its exponent
    // fallback ("1.5000e10"/"1.5000e-10") for magnitudes too large or
    // small to print plainly - not delegated to printf's own "%e"/"%g"
    // (different rounding, a leading '+' and zero-padded exponent, "1.5e+01"
    // not "1.5000e1") so a build against this host backend prints byte-
    // for-byte what the embedded build does.
    if (isnan(f)) {
        print("NAN ");
        return;
    }
    if (isinf(f)) {
        print("INF ");
        return;
    }

    if (f < 0.0f) {
        printChar('-');
        f = -f;
    }

    if (n > 9) // same clamp as printers.c's MAX_FLOAT_DECIMALS
        n = 9;
    double multiplier = 1;
    for (int i = 0; i++ < n;) multiplier *= 10;

    int exponent = 0;
    if (f != 0.0f && (f >= 1.0e9f || f < 0.5f / multiplier)) {
        while (f >= 10.0f && exponent < 38) {
            f /= 10.0f;
            exponent++;
        }
        while (f > 0.0f && f < 1.0f && exponent > -38) {
            f *= 10.0f;
            exponent--;
        }
    }

    char buf[40];
    snprintf(buf, sizeof(buf), "%.*f", n, f);
    print(buf);
    if (exponent != 0) {
        snprintf(buf, sizeof(buf), "e%d", exponent);
        print(buf);
    }
    printChar(' ');
}

// printers.c's flush() drains emitq itself (`while (qbq(emitq)) OUTPUT_FLUSH`)
// since real hardware's TX interrupt drains concurrently with the main
// loop. This host build has no such interrupt - its drain is the host
// driver's own job (see AutomatedTest/tools/tea_host_cli/main.c's
// drain_emitq_to_stdout(), wired up as OUTPUT_FLUSH itself via
// project_defs.h) - so by the time anything here could call flush(),
// emitq is already empty by construction. fflush(stdout) still matters:
// it's what actually pushes bytes already written through emitByte()
// out of libc's own stdio buffer.
void flush(void)
{
    fflush(stdout);
}

// CLI word "dump" ( a n - ): matches printers.c exactly. Was missing here
// - a real gap, not a host-vs-embedded difference like printFloat's -
// found wiring up a host build of cli.c against this file for the first
// time (Tea-host, AutomatedTest/tools/tea_host_cli).
void dump(void)
{
    Cell lines = ret();
    Byte *a = (Byte *)ret();

    pdump(a, lines);
}

void pdump(unsigned char * a, unsigned int lines)
{
    while (lines--) {
        printCr();
        printHex((unsigned int)(long)a);
        print(":");
        for (int j=0; j<2; j++) {
            for (int i = j*8; i < (j+1)*8; i++)
                printHex2(a[i]);
            print(" ");
        }
        for (int j=0; j<2; j++) {
            for (int i = j*8; i < (j+1)*8; i++)
                printChar(a[i] > 31 && a[i] < 128 ? a[i] : '.');
            print(" ");
        }
        a += 16;
    }
}

void printAt(char x) {
    if ((x >= ' ' && x <= '~') || (x == 0xD || x == LF))
        printChar(x);
    else if (x == ESC)
        print("<ESC>");
    else
        print("<"), dotnb(2,2,(Byte)x,16), print(">");
}

void printAscii(char x) {
    if (x == CR)
        print("<CR>");
    else if (x == LF)
        print("<LF>");
    else
        printAt(x);
}

void printAsciiString(const char * string) {
	while (*string)
		printAscii(*string++);
}

void psdump(unsigned short * a, unsigned int lines)
{
    while (lines--) {
        printCr();
        printHex((unsigned int)(long)a);
        print(":");
        for (int i = 8; i ; i--, printHex2(*a++));
        print(" ");
    }
}

void printerval(Long s) { // s is seconds - unit less
    print(" ");
    if (s < (5 * 60))
        printDec0(s), print("s");
    else if (s < (5 * 60 * 60))
        printDec0(s / 60), print("m");
    else if (s < (5 * 60 * 60 * 24))
        printDec0(s / (60 * 60)), print("h");
	else
		printDec0(s / (60 * 60 * 24)), print("d");
}

void hbytes(void * v, Cell n) {
    Byte * b = (Byte *)v;
    for (Short i = 0; i < n; i++)
        printHex2(b[i]);
}
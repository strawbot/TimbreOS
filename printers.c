// Printer support using Timbre  Robert Chapman III  June 14, 2012

#include "byteq.h"
#include "cli.h"
#include "tea.h"
#include <string.h>
#include <math.h>

#define ESC 0x1B
#define CR 0x0D
#define LF 0x0A

void print(const char *message) { msg((char *)message); }

void printCr(void) { print("\n"); }

void printChar(unsigned char ch) { emitByte(ch); }

void tabTo(int n) {
    int len = n - getCursor();
    if (len > 0)
        while (len-- > 0) printChar(' ');
    else
        printChar(' '); // at least one space if already past tab stop
}

void cli_tabTo() { tabTo(ret()); }

static BARRAY(PAD_SIZE, pad); // safe place to format numbers

static void convert1Digit(Cell *n, Byte base)
{
    Byte c = (Byte)(*n % base);

    if (c > 9)
        c += 7;
    c += ZERO;
    writeB(c, pad);

    *n /= base;
}

static Byte * reverseB(Cell * a) {
    Byte si = 0, ei = usedB(a);
    while (si < ei) {
        Byte c = *indexB(--ei, a);
        *indexB(ei, a) = *indexB(si, a);
        *indexB(si++, a) = c;
    }
    return indexB(0, a);
}

// 0, 0 n r - for minimal field width for all digits
char *numString(Byte field, Byte digits, Cell n, Byte radix) {
    initB(pad);
	bool negative_decimal = (Integer)n < 0 && radix == 10;
	
    if (negative_decimal)
		n = (Long)(-(Integer)n);

    if (field != 0 &&
        field == digits) // if field is equal to digits fill with zeroes
        while (digits--)
            convert1Digit(&n, radix);
    else do {
        convert1Digit(&n, radix);
    } while (n != 0);
	
	if (negative_decimal)
        writeB('-', pad);

    reverseB(pad);
    writeB(0, pad);
    return (char *)indexB(0, pad);
}

char *unumString(Byte field, Byte digits, Cell n, Byte radix) {
    initB(pad);

    if (field != 0 &&
        field == digits) // if field is equal to digits fill with zeroes
        while (digits--)
            convert1Digit(&n, radix);
    else do {
        convert1Digit(&n, radix);
    } while (n != 0);
	
    reverseB(pad);
    writeB(0, pad);
    return (char *)indexB(0, pad);
}


void dotnb(Byte field, Byte digits, Cell n, Byte radix) {
    char *string = numString(field, digits, n, radix);
    Byte width = strlen(string);
    while (width++ < field)
        print(" "); // right justify
    print(string);
}

void printnHex(unsigned int digits, unsigned int n) {
    dotnb((Byte)digits, (Byte)digits, n, 16);
}

void printHex(unsigned int hex) {
    printnHex(sizeof(Cell) * 2, hex);
    printChar((Byte)' ');
}

void printnDec(unsigned int n, unsigned int dec) { dotnb(n, n, dec, 10); }

void printDec(unsigned int dec) { lit(dec), dot(); }

void printuDec(unsigned int dec) { print(unumString(0, 0, dec, 10)); }

void printDec0(unsigned int dec) { printnDec(0, dec); }

#ifdef FLOAT_DOUBLE
#define MAX_FLOAT_DECIMALS 16 // float64 holds ~16 significant digits
#define MAX_FLOAT_EXPONENT 308
typedef Octet Frac;
#else
#define MAX_FLOAT_DECIMALS 9
#define MAX_FLOAT_EXPONENT 38
typedef Long Frac;
#endif

void printFloat0(Real f, int n) {
    if (isnan(f))
        print("NAN");
    else if (isinf(f))
        print("INF");
    else {
        if (f < 0.0f) {
            print("-");
            f = -f;
        }
        // 10^n must fit a Frac; for float32 10^10 would wrap a 32-bit Long
        // and print garbage digits. float32 has only ~7 significant digits,
        // so 9 loses nothing. float64 uses a 64-bit Frac and allows 16.
        if (n > MAX_FLOAT_DECIMALS)
            n = MAX_FLOAT_DECIMALS;
        Frac multiplier = 1;
        for(Byte i=0; i++<n;)  multiplier *= 10;

        // Reciprocity with number-literal parsing (cli.c signDigits(),
        // under FLOAT_SUPPORT): that accepts exponent notation on input,
        // so output needs the same escape valve, or two things go wrong
        // below. A magnitude too large overflows the (Long)f cast into a
        // wrong integer part - silently, not an error, just a number
        // nobody would recognize. One too small (below the smallest
        // value n decimal places can even show) rounds to all zeroes,
        // indistinguishable from an actual zero. Bounded to +-38
        // iterations either way (float32's own decimal range), same
        // reasoning as cli.c's scaleByPow10().
        int exponent = 0;
        if (f != 0.0f && (f >= 1.0e9f || f < 0.5f / multiplier)) {
            while (f >= 10.0f && exponent < MAX_FLOAT_EXPONENT) {
                f /= 10.0f;
                exponent++;
            }
            while (f > 0.0f && f < 1.0f && exponent > -MAX_FLOAT_EXPONENT) {
                f *= 10.0f;
                exponent--;
            }
        }

        Long intPart = (Long)f;
        f = f - intPart;
        Frac frac = (Frac)(f * multiplier + .5f);
        if (frac >= multiplier) { // rounding carried into integer part
            intPart++;
            frac -= multiplier;
        }
        printDec0(intPart);
        if (n > 0) {
            print(".");
            if (n > 9) { // more digits than a 32-bit Cell holds: print in two parts
                dotnb(n - 9, n - 9, (Cell)(frac / 1000000000u), 10);
                dotnb(9, 9, (Cell)(frac % 1000000000u), 10);
            } else
                dotnb(n,n,(Cell)frac,10);
        }
        if (exponent != 0) {
            print("e");
            if (exponent < 0) {
                print("-");
                exponent = -exponent;
            }
            printDec0((unsigned int)exponent);
        }
    }
}
void printFloat(Real f, int n) { printFloat0(f, n); print(" "); }

void printBin(unsigned int bin) { lit(bin), dotb(); }

void printHex2(unsigned int hex) { printChar(' '); dotnb(2, 2, hex, 16); }

void printHex4(unsigned int hex) {
    printHex2((unsigned int)(hex>>8));
    printHex2((unsigned int)hex);
}

void flush(void) {
    while (qbq(emitq))
        OUTPUT_FLUSH; // sit here until empty
}

void pdump(unsigned char *a, unsigned int lines) {
    while (lines--) {
        printCr();
        printHex((unsigned int)(uintptr_t)a);
        print(":");
        for (int j=0; j<2; j++) {
            for (int i = 0; i < 8; i++)
                printHex2(a[i+j*8]);
            print(" ");
        }
        for (int j=0; j<2; j++) {
            for (int i = 0; i < 8; i++) {
                int k = i+j*8;
                printChar(a[k] > 31 && a[k] < 128 ? a[k] : '.');
            }
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
        print("<"), dotnb(2,2,x,16), print(">");
}

void printAscii(char x) {
    if (x == CR)
        print("<CR>");
    else if (x == LF)
        print("<LF>");
    else
        printAt(x);
}

void printAsciiString(char * string) {
	while (*string) {
		printAscii(*string);
        if (*string++ == LF)
            printCr();
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
	else if (s < (1 * 60 * 60 * 24 * 365))
		printDec0(s / (60 * 60 * 24)), print("d");
    else
		printDec0(s / (60 * 60 * 24 * 365)), print("y");
}

// CLI
void dump(void) { // ( a n ) command line dump
    Cell lines = ret();
    Byte *a = (Byte *)ret();

    pdump(a, lines);
}

void psdump(unsigned short * a, unsigned int lines)
{
    while (lines--) {
        printCr();
        printHex((unsigned int)(uintptr_t)a);
        print(":");
        for (int i = 8; i ; i--, printChar(' '), dotnb(4, 4, *a++, 16));
        print(" ");
    }
}

void dumps() {
    Cell lines = ret();
    Short *a = (Short *)ret();

    psdump(a, lines);
}

void hbytes(void * v, Cell n) {
    Byte * b = (Byte *)v;
    for (Short i = 0; i < n; i++)
        printHex2(b[i]);
}

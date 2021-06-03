// Printer support using Timbre  Robert Chapman III  June 14, 2012

#include "byteq.h"
#include "cli.h"
#include "tea.h"
#include <string.h>

#define ESC 0x1B
#define CR 0x0D
#define LF 0x0A

void print(const char *message) { msg((char *)message); }

void printCr(void) { print("\n"); }

void tabTo(int n) { spaces(n - getCursor()); }

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

void printChar(unsigned char ch) { emitByte(ch); }

void printHex(unsigned int hex) {
    printnHex(sizeof(Cell) * 2, hex);
    printChar((Byte)' ');
}

void printnDec(unsigned int n, unsigned int dec) { dotnb(n, n, dec, 10); }

void printDec(unsigned int dec) { lit(dec), dot(); }

void printDec0(unsigned int dec) { printnDec(0, dec); }

void printDouble(double f, int n) {
    if (f < 0.0) {
        print("-");
        f = -f;
    }
    printDec0((int)f);
    print(".");
    while (n--) {
        f *= 10;
        if (n == 0)
            f += .5;
        printDec0(((unsigned int)f) % 10);
    }
    print(" ");
}

void printFloat(float f, int n) { printDouble((double)f, n); }

void printBin(unsigned int bin) { lit(bin), dotb(); }

void printHex2(unsigned int hex) { printChar(' '); dotnb(2, 2, hex, 16); }

void flush(void) {
    while (qbq(emitq))
        OUTPUT_BLOCKED; // sit here until empty
}

void pdump(unsigned char *a, unsigned int lines) {
    while (lines--) {
        printCr();
        printHex((unsigned int)(long)a);
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
	while (*string)
		printAscii(*string++);
}

void printerval(Long s) { // s is seconds - unit-less
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
        printHex((unsigned int)(long)a);
        print(":");
        for (int i = 8; i ; i--, printChar(' '), dotnb(4, 4, *a++, 16));
        print(" ");
    }
}

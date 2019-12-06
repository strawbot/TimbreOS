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

// 0, 0 n r - for minimal field width for all digits
char *numString(Byte field, Byte digits, Cell n, Byte radix) {
    Byte b = getBase();
    setBase(radix);
    lit(n), startNumberConversion();
    if (field != 0 &&
        field == digits) // if field is equal to digits fill with zeroes
        while (digits--)
            convertDigit();
    else
        convertNumber();
    endNumberConversion();
    setBase(b);
    ret();
    return (char *)ret();
}

void dotnb(Byte field, Byte digits, Cell n, Byte radix) {
    char *string = numString(field, digits, n, radix);
    Byte width = strlen(string);
    while (width++ < field)
        print(" "); // right justify
    print(string);
}

void printHex(unsigned int hex) { lit(hex), doth(); }

void printnHex(unsigned int digits, unsigned int n) {
    dotnb((Byte)digits, (Byte)digits, n, 16);
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

void printChar(unsigned char ch) { emitByte(ch); }

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
        for (int i = 0; i < 8; i++)
            printHex2(a[i]);
        print(" ");
        for (int i = 8; i < 16; i++)
            printHex2(a[i]);
        print("  ");
        for (int i = 0; i < 8; i++)
            printChar(a[i] > 31 && a[i] < 128 ? a[i] : '.');
        print(" ");
        for (int i = 8; i < 16; i++)
            printChar(a[i] > 31 && a[i] < 128 ? a[i] : '.');
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

void printAsciiString(char * string, Long length) {
	while (*string)
		printAscii(*string++);
}

// CLI
void dump(void) { // ( a n ) command line dump
    Cell lines = ret();
    Byte *a = (Byte *)ret();

    pdump(a, lines);
}

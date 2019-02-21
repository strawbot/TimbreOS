// printf based printers  Robert Chapman III  Jun 27, 2012

#include <stdio.h>
#include "printers.h"
#include <string.h>

static int outp = 0;

void printCr(void)
{
    outp = 0;
    printf("\n");
}

void tabTo(int n)
{
    if (n > outp)
        outp += printf("%*c", (n - outp), ' ');
}

void print(const char *message)
{
    outp += printf("%s", message);
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

    if (radix == 16)
        format = "%0*.*X";
    else
        format = "%0*.*d";

    outp += printf(format, field, digits, (unsigned int)n);
}

void printHex(unsigned int hex)
{
    outp += printf("%08X ", hex);
}

void printDec(unsigned int dec)
{
    outp += printf("%d ", (int)dec);
}

void printDec0(unsigned int dec)
{
    outp += printf("%d", (int)dec);
}

void printBin(unsigned int bin)
{
    int i;

    for(i=0x80; i != 0; i>>=1)
        outp += printf("%1i",(((bin)&i) != 0));
    outp += printf(" ");
}

void printChar(unsigned char ch)
{
    outp += printf("%c",ch);
}

void printHex2(unsigned int hex)
{
    outp += printf(" %02X",hex);
}

void printFloat(double f, int n)
{
    outp += printf("%.*f ", n, f);
}

void flush(void)
{
    fflush(stdout);
}

void pdump(unsigned char * a, unsigned int lines)
{
    while(lines--) {
        printCr();
        printHex((unsigned int)(long)a);
        print(":");
        for (int j=0; j<2; j++) {
            print(" ");
            for (int i=0; i<8; i++) print(" "), printHex2(*a++);
        }
    }
}

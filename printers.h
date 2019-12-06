// Printers header file for printer functions in a library  Robert Chapman III

#include "ttypes.h"

void printCr(void);
void tabTo(int n);
void print(const char *message);
char * numString(Byte field, Byte digits, Cell n, Byte radix);
void dotnb(Byte field, Byte digits, Cell n, Byte radix);
void printHex(unsigned int hex);
void printnHex(unsigned int n, unsigned int hex);
void printnDec(unsigned int n, unsigned int dec);
void printDec(unsigned int dec);
void printDec0(unsigned int dec);
void printFloat(float dec, int n);
void printDouble(double dec, int n);
void printBin(unsigned int bin);
void printChar(unsigned char ch);
void printHex2(unsigned int hex);
void flush(void);
void printAt(char x);
void printAscii(char x);
void printAsciiString(char * string);
void pdump(unsigned char * a, unsigned int lines);

#ifndef PRINTERS_H
#define PRINTERS_H
#define printHex4(hex) printHex2((unsigned int)(hex>>8)), printHex2((unsigned int)hex)
#endif

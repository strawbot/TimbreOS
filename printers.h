// Printers header file for printer functions in a library  Robert Chapman III

#include "ttypes.h"

void printCr(void);
void maybeCr();
void tabTo(int n);
void print(const char *message);
char * numString(Byte field, Byte digits, Cell n, Byte radix);
void dotnb(Byte field, Byte digits, Cell n, Byte radix);
void printHex(unsigned int hex);
void printnHex(unsigned int n, unsigned int hex);
void printnDec(unsigned int n, unsigned int dec);
void printDec(unsigned int dec);
void printDec0(unsigned int dec);
void printuDec(unsigned int dec);
void printFloat0(float f, int n);
void printFloat(float dec, int n);
void printBin(unsigned int bin);
void printBin0(unsigned int bin);
void printChar(unsigned char ch);
void printHex2(unsigned int hex);
void printHex4(unsigned int hex);
void flush(void);
void printAt(char x);
void printAscii(char x);
void printAsciiString(const char * string);
void printerval(Long n);
void pdump(unsigned char * a, unsigned int lines);
void psdump(unsigned short * a, unsigned int lines);
void hbytes(void * a, Cell n);

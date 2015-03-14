// Printer support using Timbre  Robert Chapman III  June 14, 2012

#include "botkernl.h"
#include "kernel.h"
#include "library.h"
#include "printers.h"
extern Byte base_;
void sendeq(void);

void print(const char *message)
{
	msg((char *)message);
}

void printHex(unsigned int hex)
{
	lit(hex), DOT_H();
}

void printnHex(unsigned int n, unsigned int hex)
{
	if (n)
		dotnb((Byte)n, (Byte)n, hex, 16);
	else
	{
		Byte b = base_;
		HEX();
		lit(hex), lit(0), DOT_R();
		base_ = b;
	}
}

void printDec(unsigned int dec)
{
	lit(dec), DOT();
}

void printDec0(unsigned int dec)
{
	lit(dec), lit(0), DOT_R();
}

void printBin(unsigned int bin)
{
	lit(bin), DOT_B();
}

void printChar(unsigned char ch)
{
	lit(ch), EMIT();
}

void printHex2(unsigned int hex)
{
	dotnb(3, 2, hex, 16);
}

void flush(void)
{
	sendeq();
}

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

void printnDec(unsigned int n, unsigned int dec)
{
	lit(dec), lit(n), DOT_R();
}

void printDec(unsigned int dec)
{
	lit(dec), DOT();
}

void printDec0(unsigned int dec)
{
	lit(dec), lit(0), DOT_R();
}

void printFloat(float f, int n)
{
	printDec0((int)f);
	print(".");
	while (n--) {
		f *= 10;
		printDec0(((unsigned int)f)%10);
	}
	print(" ");
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

void pdump(unsigned char * a, unsigned int lines)
{
	while(lines--) {
		print("\n");
		printHex((unsigned int)(long)a);
		print(":");
		for (int j=0; j<2; j++) {
			print(" ");
			for (int i=0; i<8; i++) print(" "), printHex2(*a++);
		}
	}
}

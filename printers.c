// Printer support using Timbre  Robert Chapman III  June 14, 2012

#include "cli.h"

void sendeq(void);

void dotnb(Byte field, Byte digits, Cell n, Byte radix)
{
	Byte b = getBase();

	setBase(radix);
	spaces(field - digits);
	lit(n), startNumberConversion();
	if (field) // if field is not zero print specific number of digits
		while(digits--)
			convertDigit();
	else
		convertNumber();
	endNumberConversion(), type();
	setBase(b);
}

void print(const char *message)
{
	msg((char *)message);
}

void printHex(unsigned int hex)
{
	lit(hex), doth();
}

void printnHex(unsigned int digits, unsigned int n)
{
	dotnb((Byte)digits, (Byte)digits, n, 16);
}

void printnDec(unsigned int n, unsigned int dec)
{
	dotnb(n, n, dec, 10);
}

void printDec(unsigned int dec)
{
	lit(dec), dot();
}

void printDec0(unsigned int dec)
{
	printnDec(0, dec);
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
	lit(bin), dotb();
}

void printChar(unsigned char ch)
{
	emitByte(ch);
}

void printHex2(unsigned int hex)
{
	dotnb(3, 2, hex, 16);
}

void flush(void)
{
	while (qbq(emitq))
		OUTPUT_BLOCKED;  // sit here until empty

}

void pdump(unsigned char * a, unsigned int lines)
{
	while(lines--) {
		print("\n");
		printHex((unsigned int)(long)a);
		print(":");
		for (int i=0; i<16; i++) printHex2(a[i]);
		print("  ");
		for (int i=0; i<16; i++) printChar(a[i]>31 && a[i]<128 ? a[i] : '.');
		a += 16;
	}
}

void dump(void) // command line dump
{
	Byte * a;
	Cell lines;
	
	lines = ret();
	a = (Byte *)ret();
	pdump(a, lines);
}

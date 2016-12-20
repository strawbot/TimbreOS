// MQX printers  Robert Chapman III  Jun 27, 2012

#include <stdio.h>
#include "printers.h"
 
void print(const char *message)
{
	printf("%s", message);
}

void printHex(unsigned int hex)
{
	printf("%08X ", hex);
}

void printDec(unsigned int dec)
{
	printf("%d ", dec);
}

void printDec0(unsigned int dec)
{
	printf("%d", dec);
}

void printBin(unsigned int bin)
{
	int i;

	for(i=0x80; i != 0; i>>=1)
		printf("%1i",(((bin)&i) != 0));
	printf(" ");
}

void printChar(unsigned char ch)
{
	printf("%c",ch);
}

void printHex2(unsigned int hex)
{
	printf(" %02X",hex);
}

void printFloat(float f, int n)
{
    printf("%.*f ", n, f);
}

void flush(void)
{
	fflush(stdout);
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

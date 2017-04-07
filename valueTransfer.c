// Value transfer to/from byte arrays and basic types  Robert Chapman III  Oct 2, 2012
// Note that Long can be 64bit depending on platform. Whole is 32 bit.
// Big endian arrays. Most significant byte is the first byte in the array

#include "valueTransfer.h"

void wordToBytes(Word l, Byte *b)
{
	b[0] = (Byte)(l >> 8);
	b[1] = (Byte)(l);
}

Word bytesToWord(Byte *b)
{
	return ( (Word)b[0] << 8 | (Word)b[1] );
}

void wholeToBytes(Whole l, Byte *b)
{
    Byte power = sizeof(Whole), index = 0;

    while (power--)
        b[index++] = (Byte)(l >> power*8);
}

Whole bytesToWhole(Byte *b)
{
    Byte power = sizeof(Whole), index = 0;
    Whole out = 0;

    while (power--)
        out |= (Whole)b[index++] << power*8;

    return out;
}

void longToBytes(Long l, Byte *b)
{
    Byte power = sizeof(Long), index = 0;

    while (power--)
        b[index++] = (Byte)(l >> power*8);
}

Long bytesToLong(Byte *b)
{
    Byte power = sizeof(Long), index = 0;
    Long out = 0;

    while (power--)
        out |= (Long)b[index++] << power*8;

    return out;
}

void octetToBytes(Octet l, Byte *b)
{
	int h = sizeof(Octet);

	while (h--)
		*b++ = (Byte)(l >> (h * 8));
}

Octet bytesToOctet(Byte *b)
{
	Octet n = 0;
    Long h = sizeof(Octet);

	while (h--)
        n |= (Octet)(*b++) << (h * 8);
	return n;
}

// Endian exchange - unsigned!
Short shortEndSwap(Short data)
{
	return (data<<8) | (data>>8);
}

Whole wholeEndSwap(Whole data)
{
	return   ( (Whole)shortEndSwap( (Short)(data>>16) ) ) |
		   ( ( (Whole)shortEndSwap( (Short)data       ) )<<16 );
}

Long longEndSwap(Long data)
{
	return   ( (Long)shortEndSwap( (Short)(data>>16) ) ) |
		   ( ( (Long)shortEndSwap( (Short)data       ) )<<16 );
}

Octet octetEndSwap(Octet data)
{
	return   ( (Octet)longEndSwap( (Long)(data>>32) ) ) |
		   ( ( (Octet)longEndSwap( (Long)data       ) )<<32 );
}

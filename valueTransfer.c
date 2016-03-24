// Value transfer to/from byte arrays and basic types  Robert Chapman III  Oct 2, 2012

// Big endian arrays. Most significant byte is the first byte in the array

#include "bktypes.h"
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

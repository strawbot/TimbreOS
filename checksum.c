// Checksums  Robert Chapman  Mar 19, 2015

// Fletcher Checksum: 
/*
  Initial values:	sum1 = sum2 = 0
                    For all i:
                      sum1 = sum1 + Di
                      sum2 = sum2 + sum1
: cs >r >r 0 0 r> r> for dup 1 + >r c@ + dup >r + r> r> next
] drop 0xffff and swap 0x10000 * or .h ;
0 113993 cs AA6926F7 
*/

#include "checksum.h"

Long fletcher32Byte(Byte data, Long checksum)
{
	Short sum1 = (Short)(checksum), sum2 = (Short)(checksum>>16);
	
	sum2 += sum1 += data;
	return ((Long)sum2<<16) | (Long)sum1;
}

Long fletcher32(Byte *data, Long len)
{
	Short sum1 = 0, sum2 = 0;
	
	while (len--)
		sum2 += sum1 += *data++;
	return ((Long)sum2<<16) | (Long)sum1;
}

Short fletcher16(Byte *data, Long len)
{
	Byte sum1 = 0, sum2 = 0;
	
	while (len--)
		sum2 += sum1 += *data++;
	return ((Short)sum2<<8) | (Short)sum1;
}

// 8 bit crc: X8 + X5 + X4 + 1 => 0x31 or flipped is: 0x8C
Byte crc8Slow(Byte crc, Byte data)
{
	Byte i = 8;

	crc = crc ^ data;
	while (i--)
		if (crc & 0x01)
			crc = (crc >> 1) ^ 0x8C;
		else
			crc >>= 1;

	return crc;
}

// 16 bit crc: X16 + X15 + X2 + 1 => 0x8005 or flipped is: 0xA001
Short crc16Fast(Short Crc, Byte Data) // sourcer32@gmail.com
{
    static const Short CrcTable[16] = { // Nibble lookup for 0xA001 polynomial
        0x0000,0xCC01,0xD801,0x1400,0xF001,0x3C00,0x2800,0xE401,
        0xA001,0x6C00,0x7800,0xB401,0x5000,0x9C01,0x8801,0x4400 };
 
    Crc = Crc ^ Data;
 
    // Process 8-bits, 4 at a time, or 2 rounds
 
    Crc = (Crc >> 4) ^ CrcTable[Crc & 0xF];
    Crc = (Crc >> 4) ^ CrcTable[Crc & 0xF];
 
    return(Crc);
}

Short crc16Slow(Short Crc, Byte Data) // slower, no table
{
	Byte i;

	Crc = Crc ^ Data;

	for(i=0; i<8; i++)
		if (Crc & 0x0001)
			Crc = (Crc >> 1) ^ 0xA001; // Polynomial used by MODBUS
		else
			Crc = (Crc >> 1); // Right shifting algorithm

	return(Crc);
}

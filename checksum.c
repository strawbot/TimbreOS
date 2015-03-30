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
// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const Byte crc8Table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

Byte crc8Fast(Byte crc, Byte data)
{
	return crc8Table[crc ^ data];
}

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

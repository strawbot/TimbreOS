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

#include "fletcher.h"

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


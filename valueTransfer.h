// conversions

#include "bktypes.h"

void wordToBytes(Word l, Byte *b);
Word bytesToWord(Byte *b);
void longToBytes(Long l, Byte *b);
Long bytesToLong(Byte *b);
void octetToBytes(Octet l, Byte *b);
Octet bytesToOctet(Byte *b);

Short shortEndSwap(Short data);
Long longEndSwap(Long data);
Octet octetEndSwap(Octet data);

#ifndef _VALUE_TRANSFER_H_
#define _VALUE_TRANSFER_H_

#define shortToBytes(l,b)	wordToBytes(l, b)
#define bytesToShort(b)		bytesToWord(b)

// for using Tyler's code
#define u16ToBytes(l, b)	wordToBytes(l, b)
#define bytesToU16(b)		bytesToWord(b)
#define u32ToBytes(l, b)	longToBytes(l, b)
#define bytesToU32(b)		bytesToLong(b)
#define u64ToBytes(l, b)	octetToBytes(l, b)
#define bytesToU64(b)		bytesToOctet(b)

#define uint8_t		Byte
#define uint16_t	Short
#define uint32_t	unsigned int
#define int8_t		char
#define int16_t		short
#define int32_t		int
#endif

//! \file Types for portability  Robert Chapman III  Feb 14, 2011

#ifndef TTYPES_H_
#define TTYPES_H_

#ifndef ATOMIC_SECTION_ENTER
#define ATOMIC_SECTION_ENTER __asm("cpsid i" : : : "memory")
#define ATOMIC_SECTION_LEAVE __asm("cpsie i" : : : "memory")
#endif

/* Mostly Byte = 8bit, Word = 16bit, Cell = 32bit */
// On 64 bit platforms long and pointer are 64 bit while int is 32 bit if it is LP64
// on windows (of course), it is LLP64 so long is 32 but pointers and long long are 64
// from "stdint.h" (subset of "inttypes.h" C99), the type uintptr_t is an unsigned int
// big enough to hold a pointer
//#undef Byte
#include "stdint.h"

#undef Long
#ifndef Byte
typedef unsigned char Byte; /* minimum memory transfer unit */
#endif
typedef unsigned short Word; /* next in size */
#ifndef uintptr_t
typedef unsigned int uintptr_t;
#endif
typedef uintptr_t Cell; /* default memory transfer unit; big enough for pointer */
typedef unsigned int Whole; /* whole numbers; 0 to finite infinity */
typedef   signed int Integer; /* for operations requiring the set of signed numbers */
typedef 		 float Real; // for floating point numbers
typedef unsigned short Short;
typedef unsigned long Long;
typedef signed long Lint;
typedef unsigned long long Octet; // really long
typedef signed long long Sock; // really long and signed

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef __PE_Types_H
#ifndef NO_BOOL
#if ( defined __STDC__ && defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L ) || (defined __cplusplus)
/* have a C99 compiler */

#include <stdbool.h>

#else

/* do not have a C99 compiler */
//typedef unsigned char boolean;
#undef bool
typedef unsigned char  bool;
#endif
#endif
#endif
//#endif

#ifndef true
#	define true 1
#	define false 0
#endif


#define u8(n)	((Byte)n)

// a C funtion address is a vector
// a vector followed by pointers to other vectors is a threaded code body
typedef void (*vector)();
typedef void (*unafun)(void*); // unary function

struct tcbody;

typedef union tcode { // threaded code
	struct tcbody * call; // threaded code body
	union tcode * branch; // inline branch
	Cell lit; // literal value
} tcode;

typedef struct tcbody{ // threaded code body
	vector ii; // inner interpreter
	tcode list[]; // points other body's inner interpreters
} tcbody;

#define Headless(function) \
	extern void function(void); \
	static tcbody _##function = {.ii = function}

// bit flag generic macros
#define clearBit(bit, flags)		flags &= ~(bit)
#define setBit(bit, flags)			flags |= (bit)
#define checkBit(bit, flags)		(flags & bit)
#define testSetBit(bit,flag,result)	(result = checkBit(bit,flag) ? false : true, setBit(bit,flag))

// for array sizing
#define elementsof(array)	(sizeof(array)/sizeof(array[0]))

#endif

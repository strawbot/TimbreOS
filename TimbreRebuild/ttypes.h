//! \file Types for portability  Robert Chapman III  Feb 14, 2011

#ifndef _TTYPES_H_
#define _TTYPES_H_

/* Mostly Byte = 8bit, Word = 16bit, Cell = 32bit */
// On 64 bit platforms long and pointer are 64 bit while int is 32 bit if it is LP64
// on windows (of course), it is LLP64 so long is 32 but pointers and long long are 64
// from "stdint.h" (subset of "inttypes.h" C99), the type uintptr_t is an unsigned int
// big enough to hold a pointer
//#undef Byte
#include "stdint.h"

// codewarrior is being stupid so this is needed
#undef uintptr_t
typedef unsigned long int glx_uintptr_t;
#define uintptr_t glx_uintptr_t

#undef Long
#ifndef Byte
typedef unsigned char Byte; /* minimum memory transfer unit */
#endif
typedef unsigned short Word; /* next in size */
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

#ifndef bool
#ifdef _Bool // C99 options in codewarrior
#define bool _Bool
#else
#include <stdbool.h>
#endif
#endif

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
typedef struct tcbody{ // threaded code body
	vector ii; // inner interpreter
	struct tcbody * list[]; // points other body's inner interpreters
} tcbody;
typedef tcbody * tcode; // threaded code

#define Headless(function) \
	extern void function(void); \
	tcbody _##function = {.ii = function}


// bit flag generic macros
#define clearBit(bit, flags)		flags &= ~(bit)
#define setBit(bit, flags)			flags |= (bit)
#define checkBit(bit, flags)		(flags & bit)
#define testSetBit(bit,flag,result)	(result = checkBit(bit,flag) ? false : true, setBit(bit,flag))

// for making code safe from other code interrupting it such as flag bits
#ifdef _MQX_
	void _int_disable(void);
	void _int_enable(void);
	#define safe(atomic)	{_int_disable(); atomic; _int_enable();}
#else
	#define safe(atomic)	atomic
#endif

#define uni(cycle)	safe(cycle)	// smaller synonym and also a prefix!

// for array sizing
#define elementsof(array)	(sizeof(array)/sizeof(array[0]))

#endif

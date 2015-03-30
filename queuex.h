/*			     Qtypes
  This queue structure is used to refer to the elements.  The size of
  storage is irrevelant but ANSI C must have a size greater than 0.
  The element, end, points to the last storage item.
*/
#ifndef QUEUE_H
#define QUEUE_H
/* Qtype definitions:   QUEUE(128,dataq)  */
#include "bktypes.h"

// generic structure
typedef struct {\
volatile	Cell *insert;\
volatile	Cell *remove;\
			Cell *end;\
volatile	Cell storage[1];\
}Qtype;

// queue specific structure
#define CQUEUE(size,name) struct {\
volatile	Cell *insert;\
volatile	Cell *remove;\
volatile	Cell *end;\
volatile	Cell storage[(size)+1];\
}name = {&name.storage[size], &name.storage[size], &name.storage[size],{0}}

/*
  To be consistant with BQUEUE the name should be passed to the service functions.
  The byte q uses an array of bytes so just passing the name works. The CQUEUE
  is a structure so passing the name pushes the whole structure onto the stack
  which is cumbersome so it must be preceded by an & to just pass by reference.
  An array name is this type of pointer - a constant pointer (to non-constant data).
  To make it passable by name without the & the name should be used to create
  a constant pointer to the queue structure:
*/
// generic pointer to specific structure
#define QUEUE(size, name) CQUEUE(size, name##_struct); \
	Qtype * const name = (Qtype *)&name##_struct

//typedef CQUEUE(0,Qtype);

 // With the addition of INITQ and EMPTYQ, a queue can be defined in bss and then initialized
 // later at run time.  Static data arrays only, not dynamic ones.
 // To put a queue into the data area instead of bss, use: const QUEUE(n,q);
 // Actually, by initializing the pointers the queue is moved from bss to data. But
 // declaring it as const would move it to flash.

#endif

void zeroq(Qtype *);
Cell q(Qtype *);
Cell p(Qtype *);
Cell queryq(Qtype *);
Cell qsize(Qtype *);
Cell qleft(Qtype *);
Cell pullq(Qtype *);
void pushq(Cell , Qtype *);
void pushqSafe(Cell , Qtype *);
Cell popq(Qtype *);
void stuffq(Cell , Qtype *);
void rotateq(Qtype *, Cell );
void transferq(Qtype *, Qtype *, Cell );


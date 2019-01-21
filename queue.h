// Cell q  Rob Chapman  Dec 4, 2009

/*
 Efficient structure for holding cells. Can be used to interface to asynchronous
 processes since pointers are independant.  I R and E are indexes.
             +-------------------v
   | i | r | e |       ...       |-|
     |   +--------^          ^
     |      pull<-|->pop     |
     |            |          |
     |            V          |
     |            q          |
     |                       |
     +-----------------------+
                       push<-|->stuff
                             |
                             V
                             p
  insert, remove and end are all offsets and not pointers. end is the length of
  the queue plus 1. insert and remove are incremented and modded by end or
  decremented and made into end-1 when decrementing past 0.
  | i | r | e | d | - length 0 actually 1
    0   1   2   3
  QOVERHEAD = 4
*/

#include "ttypes.h"

#ifndef _CELLQ_H_
#define _CELLQ_H_

#define QINSERT		0
#define QREMOVE		(QINSERT + 1)
#define QEND		(QREMOVE + 1)
#define QDATA		(QEND + 1)
#define QOVERHEAD	(QDATA + 1)

// for use in structures and enabled in init code later
#define NEW_Q(size, name)	Cell name[(size)+QOVERHEAD]
#define INIT_Q(q)	{ \
						(q)[QEND] = (Cell)(sizeof(q)/sizeof(q[0])) - 1; \
						(q)[QINSERT] = (q)[QREMOVE] = QDATA; \
					}

typedef Cell Qtype;

/*
	struct {
		...
		NEW_Q(10, newq);
	} s;
	
	INIT_Q(s.newq);
*/
#define QUEUE(size, name)	 NEW_Q(size, name) = {QDATA,QDATA,QDATA+(size)}
#define leftq(q)	 (sizeq(q) - queryq(q)) // how much is left

#endif

void zeroq(Cell *); // empty the queue
Cell q(Cell *); // copy of first item at head of queue
Cell p(Cell *); // copy of last item at end of queue
Cell queryq(Cell *); // get number of items in queue
Cell sizeq(Cell *); // get maximum number of items queue can hold
Cell pullq(Cell *); // pull item from queue
void pushq(Cell , Cell *); // push item into queue
Cell popq(Cell *); // pop item from queue
void stuffq(Cell , Cell *); // stuff item into queue
void rotateq(Cell *, Cell n); // rotate n queue items
void transferq(Cell *src, Cell *dst, Cell n); // transfer n items between queues
void deq(Cell item, Qtype *q); // remove all copies of item from the q
Long scanq(Cell item, Qtype *q); // count occurances of item in queue

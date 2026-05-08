// Cell q  Rob Chapman  Dec 4, 2009

/*
 Efficient structure for holding cells. Can be used to interface to asynchronous
 processes since pointers are independant.  I R and E are indexes.
             +-------------------v
   | i | r | e |       ...       |-|
     |   +--------^          ^
     |      pull<-|->stuff     |
     |            |          |
     |            V          |
     |            q          |
     |                       |
     +-----------------------+
                       push<-|->pop
                             |
                             V
                             p
  insert, remove and end are all offsets and not pointers. end is the length of
  the queue plus 1. insert and remove are incremented and modded by end or
  decremented and made into end-1 when decrementing past 0. insert points to 
  an empty cell for quick writes. remove points to a data cell for quick reads.
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

/* queue.h — unified header
typedef struct {
    Cell insert, remove, end;
    // data follows: Cell q[] for cellq, Byte q[] for byteq
} queue_hdr;

// Declaration macros (distinct storage types, same header)
#define QUEUE(size, name)  \
    struct { Cell insert, remove, end; Cell  q[size+1]; } name = {0,0,size}

#define BQUEUE(size, name) \
    struct { Cell insert, remove, end; Byte q[size+1]; } name = {0,0,size}

CellQueue<16> actionq;
actionq.push(5);   // true dot notation, fully inlined, zero overhead

#ifdef __cplusplus
#include <string.h>
template<size_t N>
struct CellQueue {
    void push(Cell v) { pushq(v, q); }
    Cell pull()       { return pullq(q); }
    Cell query()      { return queryq(q); }
private:
    QUEUE(N, q);
};
#endif
*/

#define leftq(q)	 (sizeq(q) - queryq(q)) // how much is left

#endif

void zeroq(Cell *); // empty the queue
Cell q(Cell *); // copy of first item at head of queue
Cell p(Cell *); // copy of last item at end of queue
Cell queryq(Cell *); // get number of items in queue
Cell sizeq(Cell *); // get maximum number of items queue can hold
Cell pullq(Cell *); // pull item from queue
void pushq(Cell , Cell *); // push item into queue
Cell fullq(Cell *q);
void writep(Cell c, Cell *q); // overwrite item in p end with c
Cell popq(Cell *); // pop item from queue
void stuffq(Cell , Cell *); // stuff item into queue
void writeq(Cell c, Cell *q); // overwrite item in q end with c
void rotateq(Cell *, Cell n); // rotate n queue items
void transferq(Cell *src, Cell *dst, Cell n); // transfer n items between queues
void deq(Cell item, Qtype *q); // remove all copies of item from the q
Long scanq(Cell item, Qtype *q); // count occurances of item in queue

// Byte q  Rob Chapman  Dec 4, 2009

/*
 Efficient structure for holding bytes. Can be used to interface to asynchronous
 processes since pointers are independant.  I R and E are byte array indices.
             +-------------------v
   | i | r | e |       ...       |-|
     |   +--------^          ^
     |      pull<-|          |
     |            |          |
     |            V          |
     |            q          |
     |                       |
     +-----------------------+
                       push<-|
                             |
                             V
                             p
  insert, remove and end are all indexes and not pointers. end is the length of
  the queue plus 1. insert and remove are incremented and modded by end or
  decremented and made into end-1 when decrementing past 0.
  | i | r | e | d | - length 0 actually 1
    0   1   2  []
*/

#include "ttypes.h"
#include "queue.h"

#ifndef _BYTEQ_
#define _BYTEQ_

typedef struct byteq {
    Cell insert;
    Cell remove;
    Cell end;
    Byte q[]; // could we use a union for byte array and cell array? Combine bq and q.
}byteq;

#define byteq_t Cell

#define BQDATA		0

#define BQUEUE(size, name) NEW_BQ(size, name) = {BQDATA,BQDATA,BQDATA+(size)}
#define BYTEQ(size, name) BQUEUE(size, name)

// for use in structures and enabled in init code later
#define NEW_BQ(size, name)	Cell name[(sizeof(byteq) + size + sizeof(Cell))/sizeof(Cell)]
#define INIT_BQ(bq)	{ \
						(bq)[QEND] = (Cell)(sizeof(bq)) - sizeof(byteq) - 1; \
						(bq)[QINSERT] = (bq)[QREMOVE] = BQDATA; \
					}
/*
	struct {
		...
		NEW_BQ(10, newbq);
	} s;
	
	setsizebq(10, s.newbq);
	 or
	INIT_BQ(s.newbq); // will be exact or bigger depending on length mod cell size
*/
#define leftbq(q)	 (sizebq(q) - qbq(q)) // how much is left

#endif

void zerobq(Cell *q);
Byte bq(Cell *q);
void pushbq(Byte b, Cell *q);
Byte pullbq(Cell *q);
Byte popbq(Cell *q);
Cell qbq(Cell *q);
Cell sizebq(Cell *q);
bool fullbq(Cell *q);
void setsizebq(Cell size, Cell * q);
void wrappedbq(Cell * q);


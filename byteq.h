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

typedef struct {
    Cell insert;
    Cell remove;
    Cell end;
    Byte q[]; // could we use a union for byte array and cell array? Combine bq and q.
}byteq;

#define BQDATA		0

// for use in structures and enabled in init code later
// by using a Cell array, the initial size of the byte array is lost as it is rounded up
// to be a multiple of a Cell. This will cause a problem with INIT_BQ as it assumes
// the byte array size is a function of the size of the data structure. When this could
// be off by up to sizeof(Cell)-1 bytes. To make it work the structure would have to be used
// with bytes as the array but then it could no longer be passed by name like an array
// unless the structure itself was an array of 1. Or it could just be noted that the
// byte array is always rounded up and let it be. The question is what to do with BQEND when
// initialized later.
//#define SET_BQSIZE(size, bq) {(bq)[BQEND] = (Byte)(BQDATA+(size)); (bq)[BQINSERT] = (bq)[BQREMOVE] = BQDATA; }
//#define INIT_BQ(bq)	{(bq)[BQEND] = (Byte)(sizeof(bq)) - 1; (bq)[BQINSERT] = (bq)[BQREMOVE] = BQDATA;}
#define NEW_BQ(size, name)	Cell name[sizeof(byteq) + (size + sizeof(Cell))/sizeof(Cell)]

/*
	struct {
		...
		NEW_BQ(10, newbq);
	} s;
	
	SET_BQSIZE(10, &s.newbq[0]);
*/
#define BQUEUE(size, name) NEW_BQ(size, name) = {BQDATA,BQDATA,BQDATA+(size)}

void zerobq(Cell *q);
Byte bq(Cell *q);
void pushbq(Byte b, Cell *q);
Byte pullbq(Cell *q);
Byte popbq(Cell *q);
Cell qbq(Cell *q);
Cell sizebq(Cell *q);
bool fullbq(Cell *q);

#define leftbq(q)	 (sizebq(q) - qbq(q)) // how much is left
#define wrappedbq(q)	q[QREMOVE] = q[QINSERT]; // align removal pointer with insert

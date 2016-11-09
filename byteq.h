// Byte q  Rob Chapman  Dec 4, 2009

/*
 Efficient structure for holding bytes. Can be used to interface to asynchronous
 processes since pointers are independant.  I R and E are byte offsets.
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
  insert, remove and end are all offsets and not pointers. end is the length of
  the queue plus 1. insert and remove are incremented and modded by end or
  decremented and made into end-1 when decrementing past 0.
  | i | r | e | d | - length 0 actually 1
    0   1   2   3
  BQOVERHEAD = 4
*/

#include "bktypes.h"

#define BQINSERT	0
#define BQREMOVE	(BQINSERT + 1)
#define BQEND		(BQREMOVE + 1)
#define BQDATA		(BQEND + 1)
#define BQOVERHEAD	(BQDATA + 1)
#define MAX_BQ_SIZE	254
/*
enum {
	BQINSERT
	BQREMOVE
	BQEND
	BQDATA
	BQOVERHEAD
};
*/
// for use in structures and enabled in init code later
#define SET_BQSIZE(size, bq) {(bq)[BQEND] = (Byte)(BQDATA+(size)); (bq)[BQINSERT] = (bq)[BQREMOVE] = BQDATA; }
#define INIT_BQ(bq)	{(bq)[BQEND] = (Byte)(sizeof(bq)) - 1; (bq)[BQINSERT] = (bq)[BQREMOVE] = BQDATA;}
#define NEW_BQ(size, name)	Byte name[(size)+BQOVERHEAD]

/*
	struct {
		...
		NEW_BQ(10, newbq);
	} s;
	
	SET_BQSIZE(10, &s.newbq[0]);
*/
#define BQUEUE(size, name)	 NEW_BQ(size, name) = {BQDATA,BQDATA,BQDATA+(size)}
// if I,R,E use absolute reference instead of z based from QDATA, then max size is 251
/* should complain if A is too big
    need to check limit:
	#if A > MAX_BQ_SIZE \
		#error BQUEUE B size of A exceeds maximum size \
	#endif \
*/
void zerobq(Byte *q);
Byte bq(Byte *q);
void pushbq(Byte b, Byte *q);
Byte pullbq(Byte *q);
Byte qbq(Byte *q);
Byte sizebq(Byte *q);
bool fullbq(Byte *q);

#define leftbq(q)	 (sizebq(q) - qbq(q)) // how much is left
#define wrappedbq(q)	q[BQREMOVE] = q[BQINSERT]; // align removal pointer with insert

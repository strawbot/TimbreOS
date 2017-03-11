// Byte queues with big pointers  Robert Chapman III  Mar 10, 2017

#include "byteq.h"

void zerobq(Cell *q)  // empty the queue
{
    byteq * bq = (byteq *)q;

    bq->insert = bq->remove = bq->end;
}

Byte bq(Cell *q) // return copy of oldest element
{
    byteq * bq = (byteq *)q;

    return bq->q[bq->remove];
}

void pushbq(Byte b, Cell *q) // push an element into the q
{
    byteq * bq = (byteq *)q;

    bq->q[bq->insert] = b;
    if (bq->insert == BQDATA)
        bq->insert = bq->end;
    else
        bq->insert--;
}

Byte popbq(Cell * q) // pop item from queue
{
    byteq * bq = (byteq *)q;

    bq->insert++;
    if ( bq->insert > bq->end )
            bq->insert = BQDATA;
    return bq->q[bq->insert];
}

Byte pullbq(Cell *q) // pull oldest element from the q
{
    byteq * bq = (byteq *)q;
    Byte b = bq->q[bq->remove];

    if (bq->remove == BQDATA)
        bq->remove = bq->end;
    else
        bq->remove--;
    return b;
}

Cell sizebq(Cell *q) // return size of q
{
    byteq * bq = (byteq *)q;

	return bq->end - BQDATA;
}

Cell qbq(Cell *q) // query #elements in q
{
    byteq * bq = (byteq *)q;

	if (bq->insert <= bq->remove)
		return bq->remove - bq->insert;

	return bq->remove + sizebq(q) + 1 - bq->insert;
}

bool fullbq(Cell *q) // true if q is full
{
	return qbq(q) == sizebq(q);
}

void setsizebq(Cell size, Cell * q)
{
	byteq * bq = (byteq *)q;

	bq->insert = bq->remove = bq->end = BQDATA + size;
}

void wrappedbq(Cell * q)
{
	byteq * bq = (byteq *)q;

	bq->remove = bq->insert;
}

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
    int n = bq->remove - bq->insert;

    if (n < 0)
        n += sizebq(q) + 1;

    return (Cell)n;
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

// an array with insert
void storeB(Byte b, Cell * a) {
    byteq * bq = (byteq *)a;
    bq->q[bq->insert] = b;
}

void writeB(Byte b, Cell * a) { // stuff item into an array
    byteq * bq = (byteq *)a;

    bq->q[bq->insert] = b;
    if (bq->insert < (bq->end - 1))
        bq->insert++;
}

Byte readB(Cell * a) { // read it out like a queue
    byteq * bq = (byteq *)a;
    if (bq->remove < bq->insert)
        return bq->q[bq->remove++];
    return 0;
}

Long usedB(Cell * a) {
    return ((byteq *)a)->insert - BQDATA;
}

Long leftB(Cell * a) {
    byteq * bq = (byteq *)a;
    return bq->insert - bq->remove;
}

Byte * indexB(Long index, Cell * a) {
    byteq * bq = (byteq *)a;
    return &bq->q[index];
}

void initB(Cell * a) {
    byteq * bq = (byteq *)a;
    bq->insert = bq->remove = BQDATA;
}

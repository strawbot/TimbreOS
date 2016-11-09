/* Byte sized queue facility  Rob Chapman  Dec 4, 2009 */

#include "bktypes.h"
#include "byteq.h"

void zerobq(Byte *q)  // empty the queue
{
	q[BQINSERT] = q[BQREMOVE] = q[BQEND];
}

Byte bq(Byte *q) // return copy of oldest element
{
	return (q[q[BQREMOVE]]);
}

void pushbq(Byte b, Byte *q) // push an element into the q
{
	q[q[BQINSERT]] = b;
	if (q[BQINSERT] == BQDATA)
		q[BQINSERT] = q[BQEND];
	else
		q[BQINSERT]--;
}

Byte pullbq(Byte *q) // pull oldest element from the q
{
	Byte b = q[q[BQREMOVE]];
	
	if (q[BQREMOVE] == BQDATA)
		q[BQREMOVE] = q[BQEND];
	else
		q[BQREMOVE]--;
	return b;
}

Byte sizebq(Byte *q) // return size of q
{
	return (Byte)(q[BQEND] - (Byte)BQDATA);
}

Byte qbq(Byte *q) // query #elements in q
{
	if (q[BQINSERT] <= q[BQREMOVE])
		return (Byte)(q[BQREMOVE] - q[BQINSERT]);
	return (Byte)(q[BQREMOVE] + sizebq(q) + 1 - q[BQINSERT]);
}

bool fullbq(Byte *q) // true if q is full
{
	return (Byte)(qbq(q) == sizebq(q));
}

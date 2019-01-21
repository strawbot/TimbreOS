/* Cell sized queue facility  Robert Chapman  Mar 30, 2015 */

#include "queue.h"

void zeroq(Cell *q)  // empty the queue
{
	q[QINSERT] = q[QREMOVE] = q[QEND];
}

Cell q(Cell *q) // return copy of oldest element
{
	return (q[q[QREMOVE]]);
}

void pushq(Cell c, Cell *q) // push an element into the q
{
	q[q[QINSERT]] = c;
	if (q[QINSERT] == QDATA)
		q[QINSERT] = q[QEND];
	else
		q[QINSERT]--;
}

Cell pullq(Cell *q) // pull oldest element from the q
{
	Cell c = q[q[QREMOVE]];

	if (q[QREMOVE] == QDATA)
		q[QREMOVE] = q[QEND];
	else
		q[QREMOVE]--;
	return c;
}

Cell sizeq(Cell *q) // return size of q
{
	return (Cell)(q[QEND] - (Cell)QDATA);
}

Cell queryq(Cell *q) // query #elements in q
{
    int n = q[QREMOVE] - q[QINSERT];

    if (n < 0)
        n += sizeq(q) + 1;
    return (Cell)n;
}

Cell fullq(Cell *q) // true if q is full
{
	return (Cell)(queryq(q) == sizeq(q));
}

Cell p(Cell * q) // copy of last item at end of queue
{
	if (q[QINSERT] == q[QEND])
		return q[QDATA];
	else
		return (q[q[QINSERT]+1]);
}

Cell popq(Cell * q) // pop item from queue
{
	q[QINSERT]++;
	if ( q[QINSERT] > q[QEND] )
		q[QINSERT] = QDATA;
	return q[q[QINSERT]];
}

void stuffq(Cell c, Cell * q) // stuff item into queue
{
	q[QREMOVE]++;
	if ( q[QREMOVE] > q[QEND] )
		q[QREMOVE] = QDATA;
	q[q[QREMOVE]] = c;
}

void rotateq(Cell * q, Cell n) // rotate n queue items
{
	for(;n;n--)
		pushq(pullq(q),q);
}

void transferq(Cell * srcq, Cell * dstq, Cell n) // transfer n items between queues
{  /* transfer values from srcq to dstq */
	for(;n;n--)
		pushq(pullq(srcq),dstq);
}

void deq(Cell item, Qtype *q) { // remove all copies of item from the q
	Long n;

	n = queryq(q);
	while(n--)
	{
		Cell m = pullq(q);
		if (m != item)
			pushq(m, q);
	}
}

Long scanq(Cell item, Qtype *qi) { // count occurances of item in queue
	/*
	 scan through data in the queue without moving it or changing any
	 pointers. The region of the queue to search is either a single
	 block from 1 to the size of the queue in size or two blocks with
	 one starting at the start of the queue array and the other at the
	 end of the queue array. Either one may be from 0 to the queue size
	 minus one.

	 Here's the wrong way to do it:
	*/
	Long i = 0;
	for(Long n = queryq(qi); n; n--) {
		if (item == q(qi))
			i++;
		pushq(pullq(qi),qi);
	}
	return i;
}

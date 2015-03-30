/* Qtype declaration and access  Rob Chapman  Dec 3, 1996 */

#include "bktypes.h"
#include "queue.h"

/*
                 Queues for supporting data and control flows
    Queue structure: | insert | remove | end |  data space for circular queue |

	The first cell is the insert pointer; the second cell is the removal 
	pointer and the third cell is the end pointer.  Both pointers are moved 
	down in memory by pushq() and pullq().  popq() and stuffq() move them 
	up in memory.  When either pointer gets to the pointer for end or the 
	end pointer if going the other way, they are set to the end pointer or 
	just after the pointer to the end pointer.  When the queue is zeroed, 
	both pointers are set to the same location.  Since it contains nothing, 
	the insert pointer always points to a location where values can be 
	written.  Once a value has been written with pushq(), then it is to 
	point to another empty location.  The removal pointer points to the 
	value to be pulled and when pullq() is called, it is decremented.  
	popq() uses the insert pointer and stuffq() uses the removal pointer 
	and they go the other way.  The structure of the queue is most 
	efficient for the use of pushq(), pullq() and q().  Also if you ask for 
	n cells in the queue, you actually get n+1.  This serves as an overflow 
	for the case when you say 0 QUEUE and it also allows the queue to 
	contain n items before it is full as opposed to n-1 items.

      +-----------------------------+
      |         +--------------+    |
      |         |              V    V
  | insert | remove | end |  data cells... | last data cell |
                       |                   ^
                       +-------------------+
    Note: pointers may not be the same size as a data cell.
*/

void zeroq(Qtype *q) /* initialize a queue */
{	
	q->remove = q->insert = &q->storage[0];
}

Cell q(Qtype *q)   /* return most oldest value pushed to queue */
{
	return(*q->remove);
}

Cell p(Qtype *q) /* return most recent value pushed to queue */
{
	if ( q->insert == q->end )
		return(q->storage[0]);
	else
	   return(*(q->insert + 1));
}

Cell queryq(Qtype *q)  /* return the size of a queue */
{
	if (q->remove < q->insert )
		return (Cell)( (q->end - q->insert) + (q->remove - &q->storage[0]) + 1);
	else
		return (Cell)(q->remove - q->insert );
}

Cell qsize(Qtype *q)  /* return the size of an empty queue */
{
	return (Cell)(q->end - &q->storage[0]);
}

Cell qleft(Qtype *q)  /* return what's left of a queue */
{
	return(qsize(q) - queryq(q));
}

Cell pullq(Qtype *q)  /* |queued item|>value  value is dequeued */
{
	Cell value = *q->remove--;
	if ( q->remove < &q->storage[0] )
		q->remove = q->end;
	return(value);
}

void pushq(Cell value, Qtype *q) /* value>|queued items|  value is queued */
{
	*q->insert-- = value;
	if ( q->insert < &q->storage[0] )
		q->insert = q->end;
}

Cell popq(Qtype *q) /* value<|ueued items|  value is dequeued */
{
	if ( q->insert == q->end )
	{
		q->insert = &q->storage[0];
		return q->storage[0];
	}
	return(*++q->insert);
}

void stuffq(Cell value, Qtype *q) /* |queued item|<value value is queued */
{
	if ( q->remove == q->end )
	{
		q->remove = &q->storage[0];
		q->storage[0] = value;
	}
	*++q->remove = value;
}

void rotateq(Qtype *q, Cell n) /* rotate n values in the queue */
{
	for(;n;n--)
		pushq(pullq(q),q);
}

void transferq(Qtype *srcq, Qtype *dstq, Cell n)
{  /* transfer values from srcq to dstq */
	for(;n;n--)
		pushq(pullq(srcq),dstq);
}

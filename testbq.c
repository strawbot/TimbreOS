// test bq  Rob Chapman  Apr 4, 2011

#include "byteq.h"

#define tsize 5
Byte qitem, pitem;

void testbq(void)
{
	Byte i, n = tsize;
	BQUEUE(tsize, tq)
	// test 1
	qitem = pitem = 0;
	while(n)
	{
		msg(".");
		lit(n), DOT();
		zerobq(tq);

		for (i=0; i<n; i++)
			pushbq(qitem++, tq);
		lit(qbq(tq)), DOT();
		pitem = bq(tq);
		for (i=0; i<tsize*2; i++)
		{
			Byte b;
			msg(",");
			pushbq(qitem++, tq);
			b = pullbq(tq);
			if (b != pitem)
			{
				_CR(), msg("fail item test:");
				_CR(), lit(b), DOT(), lit(pitem), DOT();
				pitem = bq(tq);
				lit(tq), lit(1), dump();
				return;
			}
			else
				pitem++;
			if (qbq(tq) != n)
			{
				msg("fail qsize test: "), lit(n), DOT(), lit(qbq(tq)), DOT();
				return;
			}
		}
		n--;
	}
	msg("done.");
}
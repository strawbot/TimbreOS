/* Dummy definitions for Timbre library  Rob Chapman  Jun 8, 2011
  modify as needed.
  
Need to define: - all of type: void (void)

getInput - must fill kq
sendeq - must empty eq

can be dummied:
setup_io - take care of any io initialization

led on and offs:
greenOff - power
greenOn
yellowOff - input
yellowOn

initApp - if app needs inits
initTimeStamp - for time stamp initialization
*/
#include "botkernl.h"
#include "byteq.h"
#include "localio.h"

// Environment
static char space[MSIZE];
Cell *sp, *rp;  /* Stack pointers */
thread *ip, tick; /* Indirect-threaded code pointers */

// Text output using queue and blocking if full
Cell recursed; // incremented if we recursed when we should never do
static enum {WAITING, RUNNING } talkState;

void initializeTalk(void)
{
    rp0_ = rp = (Cell *)(&space[MSIZE]);
	sp0_ = sp = rp0_ - RCELLS;
	dp_ = (Cell)&space[0];
	setup_io();
	RESET();
	talkState = RUNNING;
}

void safe_emit(Byte c) // c -  check queue for overflow
{
	static Byte alreadyHere = 0; // prevent overwrites

	if ((sizebq(eq) - qbq(eq)) < 1)
	{
		if (alreadyHere) // support blocking on first writer
			return;
		alreadyHere = 1;
		sendeq(); // sit here until sent
		alreadyHere = 0;
	}
	pushbq(c, eq);
}

void talkStateMachine(void)
{
	if (qbq(eq) != 0) // check output from kernel
		sendeq();
	else
	{
		getInput();
		if (qbq(kq)) // check input to kernel
		{
			yellowOn(); // blink yellow LED when processing text
			switch(talkState)
			{
				case RUNNING:	talkState = WAITING; // avoid recursion
								COLLECTOR();
								talkState = RUNNING;
								break;
				case WAITING:	recursed++;
								talkState = RUNNING;
								break;
			}
		}
		else
			yellowOff();
	}
}

void keyin(Byte c) // called from interrupt
{
	pushbq(c, kq);
}

void setup_io(void)  // take care of any io initialization
{ }

void restore_io(void) {}

void getInput(void)  // must fill kq
{
    int c = timbreGetChar();
 
	if (c != -1)
	{
		if (c == 0xA)
			c = 0xD;
		keyin((Byte)c);
	}
}

void sendeq(void)  // must empty eq
{
Byte c;
while (qbq(eq))
    {
       c = pullbq(eq);
       if (c != 0xD)
        timbrePutChar(c);
    }
}

void txUartString(Byte *s)
{
    timbrePutString(s);
}

// Leds
void greenOff(void)  // power
{ }

void greenOn(void)
{ }

void yellowOff(void)  // input
{ }

void yellowOn(void)
{ }

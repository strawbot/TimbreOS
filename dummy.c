// Dummy definitions for Timbre library  Rob Chapman  Jun 8, 2011

/*
	For command line interface - need a main loop plus this file and:
	byteq.c
	ii.c
	kernel.c
	library.c
	wordlist.c

	Example main loop using debugger for IO:	
		void timbrePutChar(Byte c)
		{
			debug_fputc(c, DEBUG_STDOUT);
		}

		int timbreGetChar(void)
		{
			return debug_fgetc(DEBUG_STDIN);
		}

		int main(void)
		{
			debug_printf("Timbre Command Line\n");
			while(true)
				timbreTalk();
			debug_exit(0);
			return 0;
		}

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
static enum {INITIALIZING, WAITING, RUNNING } talkState = INITIALIZING;

void timbreTalk(void)
{
	switch(talkState)
	{
	case INITIALIZING:
		rp0_ = rp = (Cell *)(&space[MSIZE]);
		sp0_ = sp = rp0_ - RCELLS;
		dp_ = (Cell)&space[0];
		setup_io();
		RESET();
		talkState = RUNNING;
		break;
	case RUNNING:
		talkState = WAITING; // avoid recursion via outputs
		COLLECTOR();
		talkState = RUNNING;
		break;
	case WAITING:
		recursed++;
		talkState = RUNNING;
		break;
	}

	if (qbq(eq) != 0) // check output from kernel
		sendeq();
	else {
		getInput();
		if (qbq(kq)) // check input to kernel
			yellowOn(); // blink yellow LED when processing text
		else
			yellowOff();
	}
}

void safe_emit(Byte c) // c -  check queue for overflow
{

	if ((sizebq(eq) - qbq(eq)) < 1) {
		static Byte alreadyHere = 0; // prevent overwrites
		
		if (alreadyHere) // support blocking on first writer
			return;
		alreadyHere = 1;
		sendeq(); // sit here until sent
		alreadyHere = 0;
	}
	pushbq(c, eq);
}

void keyin(Byte c) // called from interrupt
{
	pushbq(c, kq);
}

void setup_io(void)  // take care of any io initialization
{ }

void restore_io(void)
{ }

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
	while (qbq(eq)) {
       Byte c = pullbq(eq);
       if (c != 0xD)
        timbrePutChar(c);
    }
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

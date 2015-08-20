/* Local platform specific io definitions  Rob Chapman  Feb 19, 1994 */
// Copyright TimbreWorks Engineering 1990-2005

#include <stdio.h>		/* ANSI standard io */
#include <stdlib.h>

#include "botkernl.h"
#include "globals.h"
#include "byteq.h"
#include "kernel.h"
#include "localio.h"

/*
 * Memory model:
 *  | return stack     v |
 *  | parameter stack  v |
 *  | free             ^ |
 */

// space for runtime
static char space[MSIZE];

// Text output using queue and blocking if full
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

// Text input
Byte kqleft(void)  // how much room left
{
	return (Byte)(sizebq(kq) - qbq(kq));
}

void reconstitute(void)
{
	yellowOff();
    rp0_ = (Cell *)(&space[MSIZE]);
	sp0_ = rp0_ - RCELLS;
	dp_ = (Cell)&space[0];
	RESET_KERNEL();
}

void keyin(Byte c) // called from interrupt
{
	pushbq(c, kq);
}

// UART interpreter

enum {RUNNING, WAITING } talkState = WAITING;

void initializeTalk(void)  /* -- */
{
    rp0_ = rp = (Cell *)(&space[MSIZE]);
	sp0_ = sp = rp0_ - RCELLS;
	dp_ = (Cell)&space[0];
	setup_io();
	RESET_KERNEL();
	talkState = RUNNING;
}

void talkStateMachine(void)
{
	if (qbq(eq) != 0) // check output from kernel
		sendeq();
	else
	{
		getInput();
		if (qbq(kq)) // check input to kernel
			yellowOn(); // blink yellow LED when processing text
		else
			yellowOff();
	}
}

extern Byte prompt_[];

void set_prompt(Cell address)
{
	if (address)
	{
		lit(address);
		START_NUMBER_CONVERSION();
		lit(' '), HOLD();
		lit(':'), HOLD();
		CONVERT_DIGIT(), CONVERT_NUMBER();
		lit('M'), HOLD();
		END_NUMBER_CONVERSION();
		prompt_[0] = (Byte)*_DROP_;
		memcpy(&prompt_[1], (Byte *)*_DROP_, prompt_[0]);
	}
	else
		memcpy(&prompt_[0], "\005C00: ", 6);
	if (debug)
		lit(13), EMIT(), DOT_PROMPT();
}

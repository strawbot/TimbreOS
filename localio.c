/* Local platform specific io definitions  Rob Chapman  Feb 19, 1994 */
// Copyright TimbreWorks Engineering 1990-2005

#include <stdio.h>		/* ANSI standard io */
#include <stdlib.h>

#include "botkernl.h"
#include "globals.h"
#include "byteq.h"
#include "kernel.h"
#include "localio.h"
#include "machines.h"

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

void initializeTalk(void)  /* -- */
{
    rp0_ = rp = (Cell *)(&space[MSIZE]);
	sp0_ = sp = rp0_ - RCELLS;
	dp_ = (Cell)&space[0];
	RESET_KERNEL();
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

void timbreTalk(void)
{
	static enum {INITIALIZING, WAITING, RUNNING } talkState = INITIALIZING;
	static Cell recursed = 0; // incremented if we recursed when we should never do

	switch(talkState)
	{
	case INITIALIZING:
		initializeTalk();
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
}

// Run the CLI
void timbreTalkMachine(void)
{
    timbreTalk();
	activate(timbreTalkMachine);
}

void sendeq(void)
{
	while (fullbq(eq))
		runMachines();
}
/* Combine terminators here for ending programs  Rob Chapman  May 9, 1993 */
// Copyright TimbreWorks Engineering 1990-2005
#include <stdlib.h>
#include "botkernl.h"
#include "kernel.h"
#include "machines.h"
#include "localio.h"

void initApp(void);
void end_program(char *message);
void initTimeStamp(void);
void initializeTalk(void);

static enum { INIT_TALK, RUNNING } talkState = INIT_TALK;

static void talkMachine(void);
void talkMachine(void)
{
	talkStateMachine();
	activate(talkMachine);
}

void timbreStartupMessage(void);
void timbreStartupMessage(void) // delay output of text until resources in place
{
		#if (APP_DEVICE == 1)
			msg("Host Timbre\n");
		#else
			msg("Embedded Timbre");
		#endif
		DOT_PROMPT();
}

void timbre(void)
{
	if (talkState == INIT_TALK)
	{
		initMachines();
		initTimeStamp();
		greenOff();
		initializeTalk();
		talkState = RUNNING;
		greenOn();
		timbreStartupMessage();
		initApp();
		activateOnce(talkMachine);
	}
	else
		runMachines();
}

/* called when program is ending for tying up loose ends */
void end_program(char *message)
{
	msg(message);
	restore_io();
}

#ifdef APP_DEVICE
int main(void);

int main(void)
{
	while(lastkey != ESCAPE)
		timbre();
	end_program("all done\n");
	return 0;
}
#endif


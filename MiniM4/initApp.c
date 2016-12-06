
#include "machines.h"
#include "version.h"
#include "link.h"

void greenOn(void);
void blinkBlinkMachine(void);
void initTalkPort(void);
void autoEchoOn(void);
void timbreTalk(void);
void timbreTalkMachine(void);
void DOT_PROMPT(void);
void initWaveforms(void);

void initApp(void)
{
	initMachines();
	greenOn();
 	activateOnce(blinkBlinkMachine);
 	initTalkPort();
 	autoEchoOn();
  	timbreTalk();
 	activateOnce(timbreTalkMachine);
 	setBuildPrompt();
    showVersion();
 	DOT_PROMPT();
	initWaveforms();
}


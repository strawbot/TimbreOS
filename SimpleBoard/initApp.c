
#include "machines.h"
#include "version.h"
#include "link.h"

void greenOn(void);
void blinkBlinkMachine(void);
void initSerialLink(void);
void initSfp(void);
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
	initSerialLink();
// 	initSfp();
 	autoEchoOn();
  	timbreTalk();
 	activateOnce(timbreTalkMachine);
 	setBuildPrompt();
    showVersion();
 	DOT_PROMPT();
	initWaveforms();
}


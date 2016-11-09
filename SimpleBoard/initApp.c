
#include "machines.h"
#include "version.h"
#include "link.h"

void timbreTalkMachine(void);
void autoEchoOn(void);
void DOT_PROMPT(void);
void blinkBlinkMachine(void);
void greenOn(void);
void greenOff(void);
void cli(void);
void initSfp(void);
void initDebugUart(void);
void timbreTalk(void);
void initSerialLink(void);

void initApp(void)
{
	initMachines();
	greenOn();
 	activateOnce(blinkBlinkMachine);

// 	initSfp();
// 	initDebugUart();
	initSerialLink();
 	autoEchoOn();
  	timbreTalk();
 	activateOnce(timbreTalkMachine);
 	setBuildPrompt();
     showVersion();
 	DOT_PROMPT();
}


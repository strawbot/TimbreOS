// declarations
#include "bktypes.h"

extern Cell dp_;
extern Byte eq[], kq[];

void initializeTalk(void);
void keyin(Byte c);
void reconstitute(void);
void safe_emit(Byte c);
void sendeq(void);
void txUartString(Byte *s);
void set_prompt(Cell address);
void talkStateMachine(void);
void greenOn(void);
void greenOff(void);
void yellowOn(void);
void yellowOff(void);
void getInput(void);
void setup_io(void);
void restore_io(void);
void COLLECTOR(void);
void RESET(void);

void timbrePutChar(Byte c);
int timbreGetChar(void);
void timbrePutString(Byte *s);
// declarations
#include "bktypes.h"

extern Cell dp_;
extern Byte eq[], kq[];

Byte kqleft(void);
void keyin(Byte c);
void reconstitute(void);
void safe_emit(Byte c);
void sendeq(void);
void set_prompt(Cell address);
void timbreTalk(void);
void greenOn(void);
void greenOff(void);
void yellowOn(void);
void yellowOff(void);
void getInput(void);
void setup_io(void);
void restore_io(void);
void COLLECTOR(void);
void RESET_KERNEL(void);

void timbrePutChar(Byte c);
int timbreGetChar(void);
void timbrePutString(const char *s);
void setTalkOut(vector talkOutPtr);
void sfpTalk(void);
void serialTalk(void);
extern "C" {
#include "timbre.h"

Cell depth();
Cell rdepth();
void rpStore();
void hpStore();
extern Byte testMemory[];
Cell padBuffer();
void zeroEmits();
Byte getEmit();
Cell numEmits();
Byte getBase();
extern bool emitOverflow;
Cell outs();
Byte * getPrompt();
Byte getCompile();
Cell getTick();
Cell getLii();
extern Byte padq[];
Byte * getTib();
void inputString(const char * string);
header ** getWordlist();
Byte * getPadq();
Byte * string(const char * s);

void emptyKeyq(void);
void inKey(Byte c);
void inString(const char * cstring);
void collector(void);
Byte getInterpretError();
void clearInterpretError();
void compileAhead(void);
void maybeCr(void);

#ifndef _SUPPORT_H_
#define _SUPPORT_H_
#define NAMES(name) PROGMEM char name[] = {""
#define NAME(s) s "\000"
#define END_NAMES };

#define BODIES(functions) vector functions[] = {
#define BODY(f) (vector)f,
#define CONSTANTBODY(f)  (vector)cii,(vector)&f,
#define CONSTANTNUMBER(n) (vector)cii,(vector)n,
#define END_BODIES };

#endif
}

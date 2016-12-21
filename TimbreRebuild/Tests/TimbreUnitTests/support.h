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
}

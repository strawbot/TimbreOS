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
extern Byte base;
extern Byte padq[];
}

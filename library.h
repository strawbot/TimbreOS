// Copyright TimbreWorks Embedded Corporation 1990, 2012
#include "bktypes.h"
#include "botkernl.h"

void BACK_TICK(void);
void debugpmsg(PGM_P m);
void DOLLARS_STORE(void);
void DOLLARS_WITHIN(void);
void DOT_B(void);
void DOT_D(void);
void DOT_H(void);
void DOT_NAME(void);
void DOT_S(void);
void dotaddr(Cell address);
void dotfaddr(Cell address);
void dotindex(Byte index);
void dotline(Byte *bytes);
void dotLongHex(Long l);
void dotn(Byte field, Byte digits, Cell n);
void dotnb(Byte field, Byte digits, Cell n, Byte r);
void dump(void);
void evaluate(Byte *string);
void list_dictionaries(void);
void NUMBER_TO_DOLLARS(void);
void number(void);
void OCT(void);
void P(void);
void PLUS_DOLLARS(void);
void pmsg(PGM_P m);
void POP(void);
void revcpy(Byte *dest, Byte *src, Byte length);
void search_dictionaries(void);
void SHIFT(void);
void STUFF(void);
void WORDS(void);
void ZERO_KEYS(void);
void ZERO_TIB(void);

// Header file for CLI

#include "ttypes.h"

#ifndef _CLI_H_
#define _CLI_H_

// parameters
#ifndef CLI_PARAMETERS  // override by defining
#define CLI_PARAMETERS

#define DCELLS 30  // number of data stack cells
#define RCELLS 30  // number of return stack cells
#define LINE_LENGTH 80 // number of characters allowed in tib
#define EMITQ_SIZE 160
#define KEYQ_SIZE 80
#define PAD_SIZE 20
#define PROMPTSTRING "timbre: "
#define CUSHION LINE_LENGTH // how much space to maintain for HERE
#define HERE_SPACE 1024 // small here space
#define OUTPUT_BLOCKED runMachines() // deal with by running machines
#endif

// stream tokens
#define BEEP 7
#define BSPACE 8
#define LFEED 10
#define CRETURN 13
#define ESCAPE 27
#define SPACE 32
#define QUOTE 34
#define ZERO 48
#define DELETE 127

// Header bits
#define NAME_BITS 0x80
#define IMMEDIATE_BITS (NAME_BITS | 0x40)
#define SMUDGE_BITS 0x20
#define HEADER_BITS (IMMEDIATE_BITS | SMUDGE_BITS)

typedef struct header {
	struct header * list;
	Byte name[];
} header;

// Non HARVAARD architectures
#define PROGMEM const
#define PGM_P const char *
#define pgm_read_byte *
#define strcmp_P(a,b) strcmp((char*)a,(char*)b)
#define strlen_P(m) strlen(m)

#endif

extern vector wordbodies[];
extern void (*constantbodies[])();
extern void (*immediatebodies[])();

// These are character arrays with a zero between strings; C inserts a final string zero - But only if there is a string
extern PROGMEM char wordnames[];
extern PROGMEM char constantnames[];
extern PROGMEM char immediatenames[];

// API
extern Cell emitq[];
extern Cell keyq[];

void absOp(void);
Cell align(Cell p);
void allot(Cell n);
void andOp(void);
void autoEchoOff(void);
void autoEchoOn(void);
void bin(void);
void byteErase(void);
void byteFetch(void);
void byteFill(void);
void byteMove(void);
void byteStore(void);
void cComma(void);
void cii(void);
void cli(void);
void colonii(void);
void comma(void);
void comment(void);
void compileIt(tcbody * t);
void convertDigit(void);
void convertNumber(void);
void count(void);
void cursorReturn(void);
void decimal(void);
void dot(void);
void dotb(void);
void dots(void);
void doth(void);
void dotd(void);
void dotPrompt(void);
void dotr(void);
void dup(void);
void emitByte(Byte c);
void emitOp(void);
void endNumberConversion(void);
void equals(void);
void evaluate(Byte *string);
void execute(void);
void executeIt(tcbody * t);
void fetch(void);
Byte getBase(void);
void greaterThan(void);
void here(void);
void hereSay(Byte * space, Cell size);
void hex(void);
void hold(void);
void interpret(void);
void keyIn(Byte c);
void leftBracket(void);
void lessThan(void);
void lii(void);
void lit(Cell n);
void literal(Cell n);
void longFetch(void);
void longStore(void);
void maxOp(void);
void minOp(void);
void minusBits(void);
void minusOp(void);
void modOp(void);
void msg(const char * m);
void negateOp(void);
void notOp(void);
void oct(void);
void orOp(void);
void over(void);
void parse(Byte c);
Byte * parseWord(Byte c);
void plusBits(void);
void plusOp(void);
void questionDup(void);
void rat(void);
void resetCli(void);
Cell ret(void);
void rfrom(void);
void righBracket(void);
void safeEmit(Byte c);
header * searchWordlist(Byte * string);
void setBase(Byte b);
void setPrompt(const char *string);
void shiftOp(void);
void shortFetch(void);
void shortStore(void);
void sign(void);
void skip(Byte c);
void slashModOp(void);
void slashOp(void);
void spaces(int n);
void spStore(void);
void starOp(void);
void startNumberConversion(void);
void store(void);
void swap(void);
void tor(void);
void type(void);
void vii(void);
void word(void);
void words(void);
void xorOp(void);
void zeroTib(void);

void compileIf(void);
void compileEndif(void);
void compileElse(void);
void compileBegin(void);
void compileAgain(void);
void compileWhile(void);
void compileRepeat(void);
void compileUntil(void);
void compileFor(void);
void compileNext(void);
void compileExit(void);

void colon(void);
void semiColon(void);
void constant(void);
void variable(void);
void resetCli(void);

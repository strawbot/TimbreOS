// Header file for CLI

#include "ttypes.h"

#ifndef _CLI_H_
#define _CLI_H_

// parameters
#if 0
#ifndef CLI_PARAMETERS  // override by defining
#define CLI_PARAMETERS

#define DCELLS 30  // number of data stack cells
#define RCELLS 30  // number of return stack cells
#define LINE_LENGTH 80 // number of characters allowed in tib
#define EMITQ_SIZE 500
#define KEYQ_SIZE 80
#define PAD_SIZE 20
#define PROMPTSTRING "timbre: "
#define CUSHION LINE_LENGTH // how much space to maintain for HERE
#define HERE_SPACE 1024 // small here space
#define OUTPUT_BLOCKED output() // deal with by running machines
#endif
#endif

void output();

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

// Header bits:
// 0x80 = normal word (compile when compiling, execute when interpreting)
// 0xC0 = immediate word (always execute, even when compiling)
// 0x20 = smudge (exclude from search while being defined)
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

struct constantCall { vector v; Byte * b; };
#endif

extern const vector wordbodies[];
extern const struct constantCall constantbodies[];
extern const vector immediatebodies[];

extern Event EmitEvent; // called when character emitted
extern Event KeyEvent; // called when key entered

// These are character arrays with a zero between strings; C inserts a final string zero - But only if there is a string
extern PROGMEM char wordnames[];
extern PROGMEM char constantnames[];
extern PROGMEM char immediatenames[];

// API
extern Cell emitq[];
extern Cell keyq[];

void absOp();
Cell align(Cell p);
void allot(Cell n);
void andOp();
bool autoEchoIs();
void autoEchoOff();
void autoEchoOn();
void bin();
void byteErase();
void byteFetch();
void byteFill();
void byteMove();
void byteStore();
void cComma();
bool cli_is_off();
void colonii();
void comma();
void comment();
void compileIt(tcbody * t);
void convertDigit();
void convertNumber();
void cursorReturn();
void decimal();
void dot();
void dotb();
void dots();
void doth();
void dotd();
void dotPrompt();
void dotr();
void drop();
// void dup(); // conflict with <unistd.h>
void emitByte(Byte c);
void emitOp();
void emptyKeyq();
void endNumberConversion();
void equals();
void error_occurred();
void evaluate(Byte *string);
void execute();
void executeIt(tcbody * t);
void fetch();
Byte get_errors();
Byte getBase();
int getCursor();
void greaterThan();
void here();
void hereSay(Byte * space, Cell size);
void hex();
void hold();
void interpret();
bool interpreting();
void keyIn(Byte c);
void leftBracket();
void lessThan();
void lii();
void lit(Cell n);
void literal(Cell n);
Byte lookup(Byte* cstring, tcbody** t);
void maxOp();
void maybeCr();
void minOp();
void minusBits();
void minusOp();
void modOp();
void msg(const char * m);
void negateOp();
void no_prompt();
void notOp();
void oct();
void orOp();
void over();
void parse(Byte c);
Byte * parseWord(Byte c);
void plusBits();
void plusOp();
char * postQuote();
void questionDup();
void quote();
void rat();
void resetCli();
Cell ret();
void rfrom();
void rightBracket();
void run_cli();
void safeEmit(Byte c);
header * searchWordlist(Byte * string);
void setBase(Byte b);
void setPrompt(const char *string);
void shiftOp();
void shortFetch();
void shortStore();
void sign();
void skip(Byte c);
void slashModOp();
void slashOp();
void spaces(int n);
void spStore();
void starOp();
void startNumberConversion();
void store();
void stringLength();
Cell stringNumber(char * cstring);
void swap();
void nap_for();
void tor();
bool toDigit(Byte* n);
void type();
void vii();
void words();
void xorOp();
void zeroTib();
void zero_errors();

void compileIf();
void compileEndif();
void compileElse();
void compileBegin();
void compileAgain();
void compileWhile();
void compileRepeat();
void compileUntil();
void compileFor();
void compileNext();
void compileExit();

void colon();
void semiColon();
void constant();
void variable();

void setCliResult(Cell n);
Cell getCliResult();

bool isCompiling();

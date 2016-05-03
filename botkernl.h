//#pragma require_prototypes off 
/* Global stuff for botKernel  Rob Chapman  May 19, 1993 */
// Copyright TimbreWorks Engineering 1990-2006
#ifndef BOTKERNL
#define BOTKERNL

/* ==== Includes for everyone ==== */
#ifndef __MQX__
//#include <stdio.h>      /* everyone needs standard io */
#endif
//#include <setjmp.h> /* for nested error handling */
#include <string.h> /* for memory operations in kernel */
//#include <types.h>
#include "queue.h"

/* ==== Global defines ==== */
#define MSIZE 1000 /* size of memory allocated */
#define DCELLS 30  /* number of data stack cells */
#define RCELLS 30  /* number of return stack cells */
#define LINELENGTH 80 /* number of characters allowed in tib */
#define SHELLS 4  /* number of environment shells */
#define BLOCK_SIZE 512 /* number of bytes in a file block */
#define MAX_NAME 31 /* maximum length of a name: power of 2 minus 1 */

#include "bktypes.h"

typedef struct { void *link; Byte name[1]; } TIE ;

// External interfaces
void timbre(void);
void autoEchoOn(void);
void autoEchoOff(void);

/* ==== Macros for primitives ==== */
#include "macros.h"

/* ==== Kernel globals ==== */
#ifdef POLLYNOT
extern unsigned long *sp asm ("g9"),*rp asm ("g10");        /* Stacks */
extern void (***ip)() asm ("g11"),(**tick)() asm ("g12");     /* Threader */
extern Cell *sp0_,*rp0_;        /* Stacks */
#else
extern Cell *sp, *rp, *sp0_,*rp0_;        /* Stacks */
extern void (***ip)(), (**tick)(); /* Indirect-threaded code pointers */
#endif

#define ESCAPE 27
extern Byte lastkey;

void setPrompt(char *);
void DOT_PROMPT(void);
#define dotPrompt() DOT_PROMPT()
void dotnb(Byte field, Byte digits, Cell n, Byte r);

/* ==== Nested shells for error protection ==== */
//extern jmp_buf *ep;   /* for saving C context */

/* ==== Debuggers ==== */
#define BadTick  end_program("bad tick.")
#define TestTick
  /* if(*(Cell *)tick&1 || ((Cell)ip^(Cell)tick)>800000) ERROR() */

/* Somewhat 'personal' way of defining resizable dictionary entries */
// Ideal way: Entry(previous,function,name); // name is a text string- how to get rid of trailing 0 tho?
#define DictEntry(n,s) extern void s(void); \
  const struct { void *link; Byte name[ n ]; vector tick; }_##s

#define DictConstant(n,s) \
  const struct { void *link; Byte name[ n ]; vector tick; vector value; }_##s

// newer and slicker
// Use at the begining of a header file: List(kernel)
// Use for each name: Header(name, cfunction, previousfunction)

#define List(name) \
extern void *_##name ; \

// no link or name just a tick and pointer for ticking
#define Headless(function) \
extern void function(void); \
const struct { vector tick; }_##function = {function};

// strings as names
#define Header(string,function,previous) \
extern void function(void); \
const struct \
{ void *link;\
Byte name; \
Byte chars[ sizeof(string) - 1];\
vector tick; }\
_##function = \
{(void *)&_##previous,\
(sizeof(string) - 1)|0x80,\
{string},\
function};

// immediate words
#define Immediate(string,function,previous) \
extern void function(void); \
const struct \
{ void *link;\
Byte name; \
Byte chars[ sizeof(string) - 1];\
vector tick; }\
_##function = \
{(void *)&_##previous,\
(sizeof(string) - 1)|0xC0,\
{string},\
function};

// constants
extern void CII(void);
#define Constant(string,function,previous) \
extern Byte function; \
const struct \
{ void *link;\
Byte name; \
Byte chars[ sizeof(string) - 1];\
vector tick; \
Cell *value; }\
_##function = \
{(void *)&_##previous,\
(sizeof(string) - 1)|0x80,\
{string},\
CII,(Cell *)&function};

// simple names
#define Header1(string,function,previous) \
extern void function(void); \
const struct \
{ void *link;\
Byte name; \
Byte chars[ sizeof(#string) - 1];\
vector tick; }\
_##function = \
{(void *)&_##previous,\
(sizeof(#string) - 1)|0x80,\
{#string},\
function};

// an everything part
union bodypart{Cell n;void *f;void **g;};
#define END_COLON 0  // plumbing

// common declarations
//void msg(char *);
// For dual space memories
#ifdef HARVAARD
//	#include <avr/pgmspace.h>
	#define msg(message) pmsg(PSTR(message))
	#define debugmsg(message) debugpmsg(PSTR(message))
	extern prog_char names[];
	extern prog_void *functions[];
#else
	#define PROGMEM const
	#define PGM_P const char *
	#define pgm_read_byte *
	#define pgm_read_byte_far *
	#define msg(message) pmsg(message)
	#define debugmsg(message) debugpmsg(message)
	#define debugmsgn(n,message) {if (debug&n) msg(message);}
	#define strlen_P(m) strlen(m)
	#define strcmp_P(a,b) strcmp((char*)a,(char*)b)
	extern char names[];
	extern void *functions[];
#endif
void pmsg(PGM_P);

#if (APP_DEVICE == 1) 
	#define initLeds()
	#define greenOn()
	#define greenOff()
	#define yellowOn()
	#define yellowOff()
	#define redOn()
	#define redOff()
#endif

extern Byte debug;
#define TIMBRE_DEBUG 0 // set to zero or nothing for release
void revcpy(Byte *dest, Byte *src, Byte length);
void txUartString(Byte *s);

void restoreInt(Short i);
Short blockInt(void); // return state of interrupts
#endif

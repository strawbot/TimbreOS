// Support functions for CLI

extern "C" {
#define FLOAT_SUPPORT

#include "queue.c"
#include "byteq.c"
#include "cli.c"
#include "timestamp.c"
#include "timeout.c"
#include "support.h"

// stacks
Cell rdepth()
{
    return queryq(returnStack);
}

}

// memory
Byte testMemory[LINE_LENGTH*5];

Cell padBuffer()
{
    return (Cell)&testMemory[LINE_LENGTH];
}

void hpStore()
{
    hereSay(testMemory, sizeof(testMemory));
}

// output
bool emitOverflow;

void zeroEmits()
{
    emptyEmitq();
    emitOverflow = false;
}

Byte getEmit()
{
    return pullbq(emitq);
}

Cell numEmits()
{
    return qbq(emitq);
}

void runMachines()
{
    emitOverflow = true;
    zerobq(emitq);
}

Cell outs()
{
    return outp;
}

Byte * getPrompt()
{
    return prompt;
}

Cell * getPadq()
{
    return padq;
}

void output() {
    if (qbq(emitq))
        printChar(pullbq(emitq));
}

// compile
Byte getCompile()
{
    return compiling;
}

Cell getTick()
{
    return (Cell)tick;
}

Cell getLii()
{
    return (Cell)&_lii.ii;
}

// parsing
Byte * getTib()
{
    return &tib.buffer[tib.in];
}

void inputString(const char * string)
{
    zeroTib();
    strcpy((char *)tib.buffer, string);
}

void restartTib()
{
    tib.in = 0;
}

// dictionaries
// Words
NAMES(wordnames)
    NAME("wordname")
END_NAMES

BODIES(wordbodies)
END_BODIES

// Immediates
NAMES(immediatenames)
    NAME("immediatename")
END_NAMES

BODIES(immediatebodies)
END_BODIES

// Constants
NAMES(constantnames)
    NAME("constantname")
END_NAMES

BODIES(constantbodies)
    CONSTANTNUMBER(12345)
END_BODIES

header ** getWordlist()
{
    return &wordlist;
}

void inKey(Byte c)
{
    pushbq(c, keyq);
}

void inString(const char * cstring)
{
    while (*cstring)
        pushbq(*cstring++, keyq);
}

void collector(void)
{
    while (qbq(keyq))
        cli();
}

Byte getInterpretError()
{
    return interpretError;
}

void clearInterpretError()
{
    interpretError = 0;
}

Byte * string(const char * s)
{
    return (Byte *)s;
}

// time
void setTime(Cell t)
{
    uptime = t;
}

// safe
void protectCli(void) { }
void unprotectCli(void) { }

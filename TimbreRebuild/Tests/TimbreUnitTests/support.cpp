// Support functions for CLI

extern "C" {
#define FLOAT_SUPPORT

#include "queue.c"
#include "byteq.c"
#include "cli.c"
#include "support.h"

// stacks
Cell depth()
{
    return queryq(dataStack);
}

Cell rdepth()
{
    return queryq(returnStack);
}

}

// memory
Byte testMemory[LINE_LENGTH*2];

Cell padBuffer()
{
    return (Cell)&testMemory[LINE_LENGTH];
}

void hpStore()
{
    hp = testMemory;
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

Byte getBase()
{
    return base;
}

Byte * getPrompt()
{
    return prompt;
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

void * getWordlist()
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
        collectKeys();
}

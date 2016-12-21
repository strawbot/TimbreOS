// Support functions for CLI

extern "C" {

#include "queue.c"
#include "byteq.c"
#include "cli.c"

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
    zerobq(emitq);
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
    return (Cell)&_lii.tick;
}

// dictionaries
#define NAMES(name) PROGMEM char name[] = {
#define NAME(s) s "\000"
#define END_NAMES };

#define BODIES(functions) vector functions[] = {
#define BODY(f) (void * const)f,
#define CONSTANTBODY(f)  (void * const)CII,(void * const)&f,
#define CONSTANTNUMBER(n) (void * const)CII,(void * const)n,
#define END_BODIES };

// Words
NAMES(wordnames)
END_NAMES

BODIES(wordbodies)
END_BODIES

// Immediates
NAMES(immediatenames)
END_NAMES

BODIES(immediatebodies)
END_BODIES

// Constants
NAMES(constantnames)
END_NAMES

BODIES(constantbodies)
END_BODIES

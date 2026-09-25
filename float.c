// Float support for CLI  Robert Chapman  Mar 5, 2017
// Floats are 32 bit by default and ride on the data stack as their raw bits.
// Built with -DFLOAT_DOUBLE they are 64 bit, too big for a 32 bit Cell, so
// the data stack instead holds a tagged index into a pool of doubles (see
// "Float pool" below). Either way scripts see one cell per float, so dup,
// swap, over, !, @, constant and variable work the same in both builds.
#include "cli.h"
#include "printers.h"
#include "project_defs.h" // may set FLOAT_POOL
#include <stdio.h>
#include <math.h>

// tools
#ifdef FLOAT_DOUBLE
#define M(fn) fn        // double libm: sin, pow, ...
#else
#define M(fn) fn##f     // single libm: sinf, powf, ... runs on the FPU
#endif
#define floatop(op) putFloat(op(getFloat()))
#define float2op(op) Real m = getFloat(); Real n = getFloat(); putFloat(n op m);

Long decimals = 4;

#ifdef FLOAT_DOUBLE
// Float pool
//
// A handle is FLOAT_TAG | generation << 8 | slot index. Floats are never
// changed in place - every result takes a new slot - so dup/over copying a
// handle is harmless: both copies mean the same value.
//
// A slot is in use while a handle to it is on the data or return stack
// (found by scanning them when the pool runs out) or while it is pinned
// by being stored into dictionary space with , or ! (see cli.c's comma()
// and store()). Pins count, and saturate at 255 as permanent.
//
// The generation changes each time a slot is reused, so a handle that
// escaped by another route (c!, cmove, a C variable) and outlived its slot
// is reported as " stale float" instead of silently reading another value.
// A cell that is not a handle at all is read as float32 bits (getFloat).
#ifndef FLOAT_POOL
#define FLOAT_POOL 32   // at most 256; slot index is 8 bits
#endif
#define FLOAT_TAG 0xF7000000u
#define PERMANENT 255

static struct {
	Real value;
	Short gen;
	Byte pins;
	Byte live;  // allocated, or found on a stack at the last collection
} pool[FLOAT_POOL];
static Byte cursor; // where the next free slot search starts

static int rawSlot(Cell c) // slot index if c looks like a current handle, else -1
{
	if ((c & ~(Cell)0xFFFFFF) != FLOAT_TAG)
		return -1;
	Cell i = c & 0xFF;
	if (i >= FLOAT_POOL || pool[i].gen != (Short)(c >> 8))
		return -1;
	return (int)i;
}

static int slotOf(Cell c) // slot index if c is a live handle, else -1
{
	int i = rawSlot(c);
	return (i >= 0 && (pool[i].live || pool[i].pins)) ? i : -1;
}

static void markLive(Cell c)
{
	int i = rawSlot(c);
	if (i >= 0)
		pool[i].live = 1;
}

static void collect(void)
{
	for (int i = 0; i < FLOAT_POOL; i++)
		pool[i].live = 0;
	cliMarkStacks(markLive);
}

static int freeSlot(void)
{
	for (int k = 0; k < FLOAT_POOL; k++) {
		int i = (cursor + k) % FLOAT_POOL;
		if (!pool[i].live && !pool[i].pins)
			return i;
	}
	return -1;
}

static Cell newFloat(Real f)
{
	int i = freeSlot();
	if (i < 0) {
		collect();
		i = freeSlot();
		if (i < 0)
			cliFault(" float pool full - resetcli ");
	}
	cursor = (Byte)((i + 1) % FLOAT_POOL);
	pool[i].live = 1;
	pool[i].gen++;
	pool[i].value = f;
	return FLOAT_TAG | (Cell)pool[i].gen << 8 | (Cell)i;
}

bool isFloat(Cell c) { return slotOf(c) >= 0; }

void floatPin(Cell c)
{
	int i = slotOf(c);
	if (i >= 0 && pool[i].pins < PERMANENT)
		pool[i].pins++;
}

void floatUnpin(Cell c)
{
	int i = slotOf(c);
	if (i >= 0 && pool[i].pins > 0 && pool[i].pins < PERMANENT)
		pool[i].pins--;
}

void floatReset(void)
{
	for (int i = 0; i < FLOAT_POOL; i++)
		pool[i].pins = pool[i].live = 0;
}

Cell floatCell(Real f) { return newFloat(f); }

Real getFloat()
{
	Cell c = ret();
	int i = slotOf(c);
	if (i >= 0)
		return pool[i].value;
	if ((c & ~(Cell)0xFFFFFF) == FLOAT_TAG)
		cliFault(" stale float - resetcli ");

	// Not a handle: read it as float32 bits, the way the 32 bit build reads
	// every float, so a plain integer given to a float word acts the same
	// in both builds.
	union {
		float f;
		uint32_t n;
	}num = {.n = (uint32_t)c};
	return num.f;
}

void putFloat(Real f)
{
	lit(newFloat(f));
}
#else
Cell floatCell(Real f)
{
    union {
    	Real f;
    	Cell n;
    }num = {.n = 0};

	num.f = f;
	return num.n;
}

Real getFloat()
{
    union {
    	Real f;
    	Cell  n;
    }num;

	num.n = ret();
	return num.f;
}

void putFloat(Real f)
{
	lit(floatCell(f));
}
#endif

void cliFdot()
{
	printFloat(getFloat(), decimals);
}

// ops
void cliPi()
{
	putFloat(4 * M(atan)(1));
}

void cliItof()  /* n -- n */
{
	Real f = (Integer)ret();
	
	putFloat(f);
}

void cliFtoi()  /* n -- n */
{
	Real f = getFloat();
	
	f += (f < 0) ? -0.5f : 0.5f;  // round up or down
	lit((Integer)f);
}

void cliFabs()  /* n -- n */
{
	floatop(M(fabs));
}

void cliExp()  /* n -- n */
{
	floatop(M(exp));
}

void cliLog()  /* n -- n */
{
	floatop(M(log10));
}

void cliLn()  /* n -- n */
{
	floatop(M(log));
}

void cliPow()  /* n m -- n */
{
	Real m = getFloat();
	Real n = getFloat();
	
	putFloat(M(pow)(n,m));
}

void cliFneg()  /* n -- n */
{
	floatop(-);
}

void cliFgreater()  /* n m -- FLAG */
{
	Real m = getFloat();
	Real n = getFloat();

	lit((Cell)(n  >  m));
}

void cliSqrt()  /* n -- n */
{
	floatop(M(sqrt));
}

void cliCbrt()  /* n -- n */
{
	floatop(M(cbrt));
}

void cliFplus()  /* n m -- n */
{
	float2op(+);
}

void cliFminus()  /* n m -- n */
{
	float2op(-);
}

void cliFstar()  /* n m -- m */
{
	float2op(*);
}

void cliFslash()  /* n m -- m */
{
	float2op(/);
}

void cliSin()  /* n --  n */
{
	floatop(M(sin));
}

void cliCos()  /* n --  n */
{
	floatop(M(cos));
}

void cliTan()  /* n --  n */
{
	floatop(M(tan));
}

void cliAsin()  /* n --  n */
{
	floatop(M(asin));
}

void cliAcos()  /* n --  n */
{
	floatop(M(acos));
}

void cliAtan()  /* n --  n */
{
	floatop(M(atan));
}

void cliAtan2()  /* n m -- m */
{
	Real m = getFloat();
	Real n = getFloat();

	putFloat(M(atan2)(n,m));
}

// Float support for CLI  Robert Chapman  Mar 5, 2017
// For double precision, use a parallel array of doubles, enough for the whole stack
// Data stack contains indexes into double array. All float operations work on the
// indexed values. f>i and i>f change the index back to an integer.
// or pointers instead of indexes. dup would be an issue since the pointer is duplicated
// and not the value.
// maybe the stack becomes pointers or double sized.
#include "cli.h"
#include "printers.h"
#include <stdio.h>
#include <math.h>

// tools
#define floatop(op) putFloat(op(getFloat()))
#define float2op(op) float m = getFloat(); float n = getFloat(); putFloat(n op m);

Long decimals = 4;

float getFloat()
{
    union {
    	float f;
    	Cell  n;
    }num;

	num.n = ret();
	return num.f;
}

void putFloat(float f)
{
    union {
    	float f;
    	Cell  n;
    }num = {f};

	lit(num.n);
}

void cliFdot()
{
    union {
    	float f;
    	Cell  n;
    }num = {.n = ret()};

	printFloat(num.f, decimals);
}

// ops
void cliPi()
{
	static float pi = 0;
	
	if (pi == 0)
		pi = 4 * atanf(1.0f);
	putFloat(pi);
}

void cliItof()  /* n -- n */
{
	float f = (Integer)ret();
	
	putFloat(f);
}

void cliFtoi()  /* n -- n */
{
	float f = getFloat();
	
	f += (f < 0.0f) ? -0.5f : 0.5f;  // round up or down
	lit((Integer)f);
}

void cliFabs()  /* n -- n */
{
	floatop(fabsf);
}

void cliExp()  /* n -- n */
{
	floatop(expf);
}

void cliLog()  /* n -- n */
{
	floatop(log10f);
}

void cliLn()  /* n -- n */
{
	floatop(logf);
}

void cliPow()  /* n m -- n */
{
	float m = getFloat();
	float n = getFloat();
	
	putFloat(powf(n,m));
}

void cliFneg()  /* n -- n */
{
	floatop(-);
}

void cliFgreater()  /* n m -- FLAG */
{
	float m = getFloat();
	float n = getFloat();

	lit((Cell)(n  >  m));
}

void cliSqrt()  /* n -- n */
{
	floatop(sqrtf);
}

void cliCbrt()  /* n -- n */
{
	floatop(cbrtf);
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
	floatop(sinf);
}

void cliCos()  /* n --  n */
{
	floatop(cosf);
}

void cliTan()  /* n --  n */
{
	floatop(tanf);
}

void cliAsin()  /* n --  n */
{
	floatop(asinf);
}

void cliAcos()  /* n --  n */
{
	floatop(acosf);
}

void cliAtan()  /* n --  n */
{
	floatop(atanf);
}

void cliAtan2()  /* n m -- m */
{
	float m = getFloat();
	float n = getFloat();

	putFloat(atan2f(n,m));
}

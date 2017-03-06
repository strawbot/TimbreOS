// Float support for CLI  Robert Chapman  Mar 5, 2017

#include "cli.h"
#include "printers.h"
#include <stdio.h>
#include <math.h>

// tools
#define floatop(op) putFloat(op(getFloat()))
#define float2op(op) float m = getFloat(); float n = getFloat(); putFloat(n op m);

float getFloat()
{
	Cell c = ret();
	return *(float *)&c;
}

void putFloat(float f)
{
	lit(*(Cell *)&f);
}

void cliFdot()
{
	Cell f = ret();

	printFloat(*(float *)&f, 4);
}

// ops
void cliPi()
{
	static float pi = 0;
	
	if (pi == 0)
		pi = (float)4.0*(float)atan(1.0);
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
	
	f += (f < 0.0) ? -0.5 : 0.5;  // round up or down
	putFloat(f);
}

void cliFabs()  /* n -- n */
{
	floatop(fabs);
}

void cliExp()  /* n -- n */
{
	floatop(exp);
}

void cliLog()  /* n -- n */
{
	floatop(log);
}

void cliPow()  /* n m -- n */
{
	float m = getFloat();
	float n = getFloat();
	
	putFloat(pow(n,m));
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
	floatop(sqrt);
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
	floatop(sin);
}

void cliCos()  /* n --  n */
{
	floatop(cos);
}

void cliTan()  /* n --  n */
{
	floatop(tan);
}

void cliAsin()  /* n --  n */
{
	floatop(asin);
}

void cliAcos()  /* n --  n */
{
	floatop(acos);
}

void cliAtan()  /* n --  n */
{
	floatop(atan);
}

void cliAtan2()  /* n m -- m */
{
	float m = getFloat();
	float n = getFloat();

	putFloat(atan2(n,m));
}

// Single and Double blinker for LED  Robert Chapman III  Jun 26, 2012

#include "timeout.h"
#include "machines.h"

void greenOff(void);
void greenOn(void);
void blink(void);
void blinkBlink(void);
void blinkBlonk(void);
void blinkBlonkMachine(void);
void initBlinkBlonk(void);

void blink(void)
{
	static struct {void (*action)(void); Long time; int next; } state[] = {
		{greenOff,	500 MILLISECONDS, 	1},
		{greenOn,	500 MILLISECONDS,	0}};
	static Timeout bto = {0};
	static int current = 0;
	
	if (checkTimeout(&bto))
	{
		state[current].action();
		setTimeout(state[current].time, &bto);
		current = state[current].next;
	}
}

void blinkBlink(void)
{
	static struct {void (*action)(void); Long time; int next; } state[] = {
		{greenOff,	700 MILLISECONDS, 	1},
		{greenOn,	100 MILLISECONDS,	2},
		{greenOff,	100 MILLISECONDS,	3},
		{greenOn,	100 MILLISECONDS,	0}};
	static Timeout bto = {0};
	static int current = 0;
	
	if (checkTimeout(&bto))
	{
		state[current].action();
		setTimeout(state[current].time, &bto);
		current = state[current].next;
	}
}

void blinkBlonk(void)
{
	static struct {void (*action)(void); Long time; int next; } state[] = {
		{greenOff,	100 MILLISECONDS, 	1},
		{greenOn,	100 MILLISECONDS,	2},
		{greenOff,	100 MILLISECONDS,	3},
		{greenOn,	700 MILLISECONDS,	0}};
	static Timeout bto = {0};
	static int current = 0;
	
	if (checkTimeout(&bto))
	{
		state[current].action();
		setTimeout(state[current].time, &bto);
		current = state[current].next;
	}
}

void blinkMachine(void)
{
	blink();
	activate(blinkMachine);
}

void blinkBlinkMachine(void)
{
	blinkBlink();
	activate(blinkBlinkMachine);
}

void blinkBlonkMachine(void)
{
	blinkBlonk();
	activate(blinkBlonkMachine);
}

void initBlinkBlonk(void)
{
	activateOnce(blinkBlonkMachine);
}
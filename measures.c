#include "timbre.h"
#include "measures.h"

void machineCounts(void); // count and time machine cycles
void machineCounts(void) // count and time machine cycles
{
	static Long timerstarted = 0;
	static Timeout timer;
	static Long cycles = 0, min=0xffffffff, avg=0, max=0;

	if (timerstarted == 0)
	{
		setTimeout(1 SEC, &timer);
		timerstarted = 1;
	}
	else
	{
		if (checkTimeout(&timer))
		{
			repeatTimeout(&timer);
			if (cycles < min)
				min = cycles;
			if (cycles > max)
				max = cycles;
			avg = (avg + min + max + cycles) / 4;
			cycles = 0;
		}
	}
	cycles++;
	activate(machineCounts);
}

void initTooling(void)
{
	activateOnce(machineCounts);
}

// time stamping to trace time delays through the code space
tstamp_t tstamps[nstamps];

void tstamp(tstamp_t *timer)
{
	timer->msec = getTime();
	timer->hwticks = get_hwticks();
}


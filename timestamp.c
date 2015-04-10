// Time  Rob Chapman  Mar 14, 2011

/* Time is based on a 32 bit number. With 1ms resolutions this is good for 47 days.
   Locally a time is kept ticking from startup. This is the basis for time for timeouts.
   For timestamps, a time reference is needed which must be set by the host.
*/

#include "timestamp.h"
#include "timeout.h"

static Long localtime; // local time is time since startup

void oneMillisecondTick(void)
{
	localtime += LOCALTIME_TICK;
	ACKNOWLEDGE_TICK;
}

void initTimeStamp(void)
{
	localtime = 0;
}

// reading time
Long getTime(void)
{
	return localtime;
}

// CLI for timing
#include "printers.h"
Long startingTime;

void showTime(void)
{
	printDec(getTime());
}

void sdotms(Long time)
{
	Long ms = time%(1 TO_SECOND);

	printDec0(time/(1 TO_SECOND));
	print(".");
	printDec0(ms/100);
	printDec0((ms%100)/10);
	printDec0(ms%10);
}

void startTime(void)
{
	startingTime = getTime();
}

void endTime(void)
{
	Long t = getTime() - startingTime;
	
	print(" Elapsed time (S.ms): ");
	sdotms(t);
}

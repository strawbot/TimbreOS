// Time  Rob Chapman  Mar 14, 2011

/* Time is based on a 32 bit number. With 1ms resolutions this is good for 47 days.
   Locally a time is kept ticking from startup. This is the basis for time for timeouts.
   For timestamps, a time reference is needed which must be set by the host.
*/

#include "timestamp.h"
#include "timeout.h"

Long localtime; // local time is time since startup

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

#ifdef DISPLAY_TIME // commands to display time
#include "botkernl.h"
#include "kernel.h"
#include "library.h"

void showTime(void)
{
	lit(getTime()), DOT();
}

// for timing
Long startingTime;

void startTime(void)
{
	startingTime = getTime();
}

void sdotms(Long time)
{
	Long ms = time%(1 SEC);

	lit(time/(1 SEC)), lit(0), DOT_R();
	msg(".");
	lit(ms/100), lit(0), DOT_R();
	lit((ms%100)/10), lit(0), DOT_R();
	lit(ms%10), lit(0), DOT_R();
}

void endTime(void)
{
	Long t = getTime() - startingTime;
	
	msg(" Elapsed time (S.ms): ");
	sdotms(t);
}
#endif

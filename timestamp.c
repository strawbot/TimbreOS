// Time  Rob Chapman  Mar 14, 2011

/* Time is based on a 32 bit number. With 1ms resolutions this is good for 47 days.
   Locally a time is kept ticking from startup. This is the basis for time for timeouts.
   For timestamps, a time reference is needed which must be set by the host.
*/

#include "timestamp.h"

static Long uptime; // local time is time since startup

void millisecondsTick(Long ms)
{
	uptime += ms;
	ACKNOWLEDGE_TICK;
}

void oneMillisecondTick(void)
{
	millisecondsTick(LOCALTIME_TICK);
}

void initTimeStamp(void)
{
	uptime = 0;
}

// reading time
Long getTime(void)
{
	return uptime;
}

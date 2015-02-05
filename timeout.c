// Timeouts  Rob Chapman  Apr 14, 2011

#include "timeout.h"
#include "timestamp.h"

bool checkTimeout(Timeout *timer) // see if it has timed out
{
	if (timer->off == true) // see if it is enalbed
		return false;
	return (bool)((getTime() - timer->timeset) > timer->timeout);
}

void setTimeout(Cell time, Timeout *timer) // set the timeout time and turn on the timeout
{
	timer->timeset = getTime(); // not set here to avoid wraparound issues
	timer->timeout = time;
	timer->off = false;
}

Integer sinceTimeout(Timeout *timer) // get time since last timeout for accurate repeated timeouts
{
	return (Integer)(getTime() - (timer->timeset + timer->timeout)); 
}

void stopTimeout(Timeout *timer) // shut off a timeout
{
	timer->off = true;
}

void startTimeout(Timeout *timer) // turn on a timeout
{
	timer->off = false;
}

void repeatTimeout(Timeout *timer) // set the timeout time for an interval after and turn on the timeout
{
	timer->timeset += timer->timeout;
	timer->off = false;
}

void timeoutWait(Cell time)
{
	Timeout timer;

	setTimeout(time, &timer);
	while (!checkTimeout(&timer))
		;
}
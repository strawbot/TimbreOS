// Timeouts  Rob Chapman  Apr 14, 2011

#include "timeout.h"
#include "cli.h"

bool checkTimeout(Timeout *timer) // see if it has timed out
{
	if (timer->off == true) // see if it is enalbed
		return true; // a timer off is condidered don
	
	Integer elapsed = getTime() - timer->timeset;
	Integer interval = (Integer)timer->timeout;

	return (timer->off = elapsed >= interval);
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
	if (checkTimeout(timer)) { // only repeat it if it has happened
		timer->timeset += timer->timeout;
		timer->off = false;
	}
}

void restartTimeout(Timeout *timer) // restart the timeout from right now
{
	timer->timeset = getTime(); // not set here to avoid wraparound issues
	timer->off = false;
}

void timeoutWait(Cell time) // timed delay loop
{
	NEW_TO(timer);

	setTimeout(time, timer);
	while (!checkTimeout(timer))
		action_slice();
}

// CLI for timing
#include "printers.h"
#include "tea.h"

static Long startingTime;

void showTime(void)
{
	printDec(getTime());
}

void sdotms(Long time)
{
	Long ms = time%(secs(1));

	printDec0(time/(secs(1)));
	print(".");
	printDec0(ms/100);
	printDec0((ms%100)/10);
	printDec0(ms%10);
}

void cliStartTime(void)
{
	startingTime = getTime();
}

Long cliEndTime(void)
{
	Long t = getTime() - startingTime;
	
	print(" Elapsed time (S.ms): ");
	sdotms(t);
	return t;
}

void cliSleep() {
    timeoutWait(secs(ret()));
}
// Timeouts  Rob Chapman  Apr 14, 2011
// Note: time is given in ticks and must be converted to milliseconds to
//       work with getTime() which is in milliseconds and 32 bit. This
//       puts a limit on timeout to 49 days
//       For 15 bit ticks per second, 32 bits is 128K seconds or 36h
#include "timestamp.h"
#include "timeout.h"
#include "cli.h"
#include "tea.h"

bool checkTimeout(Timeout *timer) // see if it has timed out
{
	if (timer->off == true) // see if it is disabled
		return true; // a disabled timer is considered done
	
	Integer elapsed = getTime() - timer->timeset;
	Integer interval = (Integer)timer->timeout;
	return (timer->off = elapsed >= interval);
}

void setTimeout(Cell time, Timeout *timer) // set the timeout time and turn on the timeout
{
	timer->timeset = getTime(); // not set here to avoid wraparound issues
	timer->timeout = to_msec(time);
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

void ms_delay(Cell ms) { // meant as a blocking delay
	NEW_TO(timer);

	setTimeout(ms, timer);
	while (!checkTimeout(timer)) {}
}

Long timeout_left(Timeout * to) { // amount of time before due in ticks
	if (!to->off) {
		Long passed = getTime() - to->timeset;
			
		if (passed < to->timeout)
			return to->timeout - passed;
	}
	return 0;
}

// CLI for timing
#include "printers.h"
#include "tea.h"

static Long startingTime;

void showTime(void)
{
	printDec(getTime());
}

void sdotms(Octet time)
{
	Short ms = time%1000;

	printDec0(time/1000);
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

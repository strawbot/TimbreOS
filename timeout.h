// Timeouts  Rob Chapman  Mar 14, 2011

#include "timestamp.h"

#ifndef timeout_h
#define timeout_h

// Note: 16 bits max 1 minute; 32 bits max 49 days; 48 bits max 8900 years; 64 bits max forever
// based in uS, 64 bits would give 584 million years; 48 bits would give 8.9 years
// 32 bits for us and 32 bits for seconds goes up to 49000 years
#define TO_MILLISECOND / LOCALTIME_TICK
#define TO_SECOND * (1000UL TO_MILLISECONDS)
#define TO_MINUTE * (60UL TO_SECONDS)
#define TO_HOUR * (60UL TO_MINUTES)
#define TO_DAY * (24UL TO_HOURS)

// and plurals for sytactic sugar
#define TO_MILLISECONDS TO_MILLISECOND
#define TO_SECONDS TO_SECOND
#define TO_MINUTES TO_MINUTE
#define TO_HOURS TO_HOUR
#define TO_DAYS TO_DAY

// and shorts
#define TO_MSECS TO_MILLISECOND
#define TO_SECS TO_SECOND
#define TO_MINS TO_MINUTE
#define TO_HOURS TO_HOUR
#define TO_DAYS TO_DAY

/* evolves:
 o switch for 16 bit, 32 bit, 64 bit support for different time length support
   - need typedef for time value then which is Short, Long, Octet
   - #define TimeoutTime ...
   - multiplications in macros needs to use proper length number U, UL, ULL or casts
 o offer MSEC(n), SEC(n), MIN(n), HOUR(n), DAY(n) macros
*/

typedef struct {
	Long timeout;	// length of timeout
	Long timeset;	// time of setting timeout
	Byte off;		// set to true to turn off timeout - reset by setting
} Timeout;

#define NEW_TO(name) Timeout name[1] /* use timeouts without & */

// Timeout support
// MEW_TO(xto);
// if (checkTimeout(xto)
// setTimeout(4 SECONDS, xto)
// sinceTimeout(xto)
// accurate timeouts: setTimeout(4 SECONDS - sinceTimeout(xto), xto)
// return time since timeout expired
//#define Timeout Long
//#define checkTimeout(timer) (timer <= getTime()) // this will fail after 49 days
//#define setTimeout(time, timer) timer = getTime() + time
//#define sinceTimeout(timer) (timer - getTime())
// Note: (getTime() - rtcto.timeset) will give elapsed time from start of timeout

#endif

bool checkTimeout(Timeout *timer);
void setTimeout(Cell time, Timeout *timer);
Integer sinceTimeout(Timeout *timer);
void stopTimeout(Timeout *timer);
void startTimeout(Timeout *timer);
void repeatTimeout(Timeout *timer);
void timeoutWait(Cell time);
void restartTimeout(Timeout *timer);

void showTime(void);
void cliStartTime(void);
void sdotms(Long time);
Long cliEndTime(void);

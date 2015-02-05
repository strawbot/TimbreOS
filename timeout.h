// Timeouts  Rob Chapman  Mar 14, 2011

#include "bktypes.h"
#include "timestamp.h"

#ifndef timeout_h
#define timeout_h

#define MSEC	/LOCALTIME_TICK
#define MSECS	MSEC
#define SEC * (1000UL MSECS)
#define SECS SEC

#define MINUTE *(60UL SECS) // need more than 16 bits for more than 1
#define MINUTES MINUTE
#define HOUR *(60UL MINUTES)
#define HOURS HOUR
#define TO_DAY *(24UL HOURS)
#define TO_DAYS TO_DAY // need more than 32 bits for more than 49 days

// alternates
#ifndef SECONDS
#define MILLISECOND MSEC
#define MILLISECONDS MSEC
#define SECOND SEC
#define SECONDS SEC
#endif

/*
#define MSEC(n) (n MSECS)
#define SEC(n) (n SECS)
#define MIN(n) (n MINUTES)
#define HOUR(n) (n HOURS)
#define DAY(n) (n DAYS)
*/

/* evolves:
 o switch for 16 bit, 32 bit, 64 bit support for different time length support
   - need typedef for time value then which is Short, Long, Octet
   - #define TimeoutTime ...
   - multiplications in macros needs to use proper length number U, UL, ULL or casts
 o offer MSEC(n), SEC(n), MIN(n), HOUR(n), DAY(n) macros
*/

// alternate for brevity and compatibility
#define TO_MSEC MILLISECOND
#define TO_MSECS MILLISECOND
#define TO_SEC SECOND
#define TO_SECS SECOND

typedef struct {
	Long timeout;	// length of timeout
	Long timeset;	// time of setting timeout
	Byte off;		// set to true to turn off timeout - reset by setting
} Timeout;

// Timeout support
// Timeout xto
// if (checkTimeout(xto)
// setTimeout(4 SECONDS, xto)
// sinceTimeout(xto)
// accurate timeouts: setTimeout(4 SECONDS - sinceTimeout(xto), xto)
// return time since timeout expired
//#define Timeout Long
//#define checkTimeout(timer) (timer <= getTime()) // this will fail after 49 days
//#define setTimeout(time, timer) timer = getTime() + time
//#define sinceTimeout(timer) (timer - getTime())


#endif

bool checkTimeout(Timeout *timer);
void setTimeout(Long time, Timeout *timer);
Integer sinceTimeout(Timeout *timer);
void stopTimeout(Timeout *timer);
void startTimeout(Timeout *timer);
void repeatTimeout(Timeout *timer);
void timeoutWait(Cell time);


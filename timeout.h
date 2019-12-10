// Timeouts  Rob Chapman  Mar 14, 2011

#include "project_defs.h"
#include "ttypes.h"

#ifndef timeout_h
#define timeout_h

// Note: 16 bits max 1 minute; 32 bits max 49 days; 48 bits max 8900 years; 64 bits max forever
// based in uS, 64 bits would give 584 million years; 48 bits would give 8.9 years
// 32 bits for us and 32 bits for seconds goes up to 49000 years

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
//#define elapsedTime(timer) (timer - getTime())
// Note: (getTime() - rtcto.timeset) will give elapsed time from start of timeout

// #define ta_usecs()
// Define ta_msecs in a modified timestamp.h file
#define ta_secs(t) (ta_msecs((t) * 1000))
#define ta_mins(t) (ta_secs((t) * 60))
#define ta_hours(t) (ta_mins((t) * 60))

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

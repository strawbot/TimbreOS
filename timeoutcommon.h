// common timeout support

#include "botkernl.h"

Long getTime(void);

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

bool checkTimeout(Timeout *timer);
void setTimeout(Long time, Timeout *timer);
Long sinceTimeout(Timeout *timer);
void stopTimeout(Timeout *timer);


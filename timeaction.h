#include "timeout.h"

#ifndef _TIMEACTION_H_
#define _TIMEACTION_H_

#define after(t, m) \
do { \
	static TimeAction ta = {.action = m, .name = #m}; \
	setTimeout(t, &ta.to); \
	timeaction(&ta); \
} while (false)

typedef struct TimeAction {
	struct TimeAction * link;
	Timeout to;
	vector action;
	const char * name;
} TimeAction;

void timeaction(TimeAction * ta);

// #define ta_usecs()
#define ta_msecs(t) (((long long)t*128)/125)
#define ta_secs(t)  (ta_msecs(t*1000))
#define ta_mins(t)  (ta_secs(t*60))
#define ta_hours(t) ((Long)ta_mins(t*60))

#endif

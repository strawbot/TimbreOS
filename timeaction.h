#include "timeout.h"

#ifndef _TIMEACTION_H_
#define _TIMEACTION_H_

#define after(t, m) \
{ \
	static TimeAction ta = {.action = m, .name = #m}; \
	setTimeout(t, &ta->to); \
	timeaction(&ta); \
}

typedef struct TimeAction {
	struct TimeAction * link;
	Timeout to;
	vector action;
	const char * name;
} TimeAction;

void timeaction(TimeAction * ta);

#endif

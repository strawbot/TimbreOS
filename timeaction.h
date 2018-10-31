#include “timeouts.h”

#ifndef _TIMEACTION_H_
#define _TIMEACTION_H_

#define after(t, m)
{
	static TimeAction ta = {action = m, .name = “#m”};
	doLater(t, &ta);
}

typedef struct TimeAction {
	struct TimeAction * link;
	Timeout to;
	vector action;
	const char * name;
} TimeAction;

void doLater(Long t, TimeAction * ta);

#endif
#include "timeout.h"
#include "queue.h"
#include <stdbool.h>

#ifndef _TIMEACTION_H_
#define _TIMEACTION_H_

#define LINK_SENTINEL  (TimeAction *)0xFACEF00D

enum {FREE_RANGE, RANCH}; // one from somewhere else or on in this collection

typedef struct TimeAction {
	struct TimeAction * link;
	vector action;
	Timeout to;
	Cell type;
} TimeAction;

#define after(t, m) \
do { \
	static TimeAction ta = {LINK_SENTINEL, m, {0}, 0}; \
	after_time(t, &ta); \
} while (false)

#define every(t, m) \
do { \
	static TimeAction ta = {LINK_SENTINEL, m, {t,0,true}, 0}; \
	every_time(&ta); \
} while (false)

void timeaction(TimeAction * ta);
void after_time(Long t, TimeAction * ta);
void every_time(TimeAction * ta);

TimeAction * timeToAction(Long time, vector action);
void stopTa(TimeAction * ta);
void stopTimeAction(vector action);

// #define ta_usecs()
#define ta_msecs(t) (((long long)t*128)/125)
#define ta_secs(t)  (ta_msecs(t*1000))
#define ta_mins(t)  (ta_secs(t*60))
#define ta_hours(t) ((Long)ta_mins(t*60))

#endif

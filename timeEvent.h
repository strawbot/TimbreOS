#include "timeout.h"
#include "queue.h"
#include <stdbool.h>
#include "event.h"

#ifndef TIMEEVENTH_
#define TIMEEVENT_H_

#define LINK_SENTINEL  (TimeEvent *)0xFACEF00D

typedef struct TimeEvent {
	Action action; // first place - inherit structure; Action * for both
	struct TimeEvent * link;
	Timeout to;
} TimeEvent;

TimeEvent * after(Long time, vector action);
TimeEvent * every(Long time, vector action);

void timeaction(TimeEvent * ta);

TimeEvent * timeToAction(Long time, vector action);
void stopTa(TimeEvent * ta);
void stopTimeAction(vector action);

// #define ta_usecs()
#define ta_msecs(t) (((long long)t*128)/125)
#define ta_secs(t)  (ta_msecs(t*1000))
#define ta_mins(t)  (ta_secs(t*60))
#define ta_hours(t) ((Long)ta_mins(t*60))

#endif

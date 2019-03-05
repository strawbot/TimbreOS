#include "event.h"
#include "queue.h"
#include "timeout.h"
#include <stdbool.h>

/*
 * Time events are events that happen according to a time
 * either relative or absolute.
 * API:
 *  after(time, action) // oneshot
 *  every(time, action) // periodic
 *  stop
 */
#ifndef TIMEEVENT_H_
#define TIMEEVENT_H_

#define LINK_SENTINEL (TimeEvent*)0xFACEF00D

typedef struct TimeEvent {
    Action action; // first place - inherit structure; Action * for both
    Timeout to;
    struct TimeEvent* link;
} TimeEvent;

#ifdef __cplusplus
// C++
extern "C"   TimeEvent* after(Long time, vector action);
extern "C++" TimeEvent* after(Long time, void* cpp_obj, unafun action);

extern "C"   TimeEvent* every(Long time, vector action);
extern "C++" TimeEvent* every(Long time, void* cpp_obj, unafun action);

extern "C"   void stopTimeEvent(vector action);
extern "C++" void stopTimeEvent(void* cpp_obj, unafun action);

extern "C"   void stopTe(TimeEvent* ta);

#else
// C
TimeEvent* after(Long time, vector action);
TimeEvent* every(Long time, vector action);
void stopTimeEvent(vector action);
void stopTe(TimeEvent* ta);

#endif

void timeaction(TimeEvent* ta);

TimeEvent* timeToAction(Long time, vector action);

// #define ta_usecs()
#define ta_msecs(t) (((long long)t * 128) / 125)
#define ta_secs(t) (ta_msecs(t * 1000))
#define ta_mins(t) (ta_secs(t * 60))
#define ta_hours(t) ((Long)ta_mins(t * 60))


#endif

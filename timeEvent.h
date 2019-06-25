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

typedef struct TimeEvent {
    Action action; // first place - inherit structure; Action * for both
    Timeout to;
    struct TimeEvent* link;
    enum {TE_FREE, TE_USED} state;
    union {
    	Long id;
    	struct {
    		Long index : 8;
    		Long tag   : 24;
    	};
    };
} TimeEvent;

#ifdef __cplusplus
extern "C" {
#endif

Long after(Long time, vector action);
Long every(Long time, vector action);
void stopTimeEvent(vector action);
void stopTe(Long teid);
void change(Long time, Long id);

#ifdef __cplusplus
}

extern "C++" {
Long after(Long time, void* cpp_obj, unafun action, const char * name = "");
Long every(Long time, void* cpp_obj, unafun action, const char * name = "");
void stopTimeEvent(void* cpp_obj, unafun action);
}

#endif

void timeaction(TimeEvent* te);

Long timeToAction(Long time, vector action);

// #define ta_usecs()
#define ta_msecs(t) ((Long)(((unsigned long long)(t) * 128) / 125))
#define ta_secs(t) (ta_msecs((t) * 1000))
#define ta_mins(t) (ta_secs((t) * 60))
#define ta_hours(t) (ta_mins((t) * 60))


#endif

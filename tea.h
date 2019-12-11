#include "clocks.h"
#include "project_defs.h"
#include "ttypes.h"

#ifndef TEA_H_
#define TEA_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BLACK_HOLE()                                                           \
    while (true)                                                               \
        ; // DEBUGGING

// time
typedef struct TimeEvent {
    struct TimeEvent *next;
    vector action;
    bool asap;
    Short dueDate; // up to 65000 ms
} TimeEvent;

// time base
Long get_uptime(); // seconds since startup
Long getTime(void);    // ms time stamp; 49 day rollover

#define msec(t) ((Long)(((Octet)(t)*ONE_SECOND) / 1000))
#define secs(t) msec(t * 1000)
#define to_msec(n) (to_secs((n)*1000))
#define to_secs(n) ((n) / ONE_SECOND)

// time
void after(Long t, vector action);
void in(Long t, vector action);

// Events
typedef vector Event[1];
void when(Event e, vector a);
void never(Event e);

// actions
#define now(action) (action)()
void later(void (*a)());
void run();

// inactions
void no_action();
void stop_te(vector v);
void stop_action(vector v);
void stop(vector v); // all places;

void init_te();

// C++ support
#ifdef __cplusplus
}
// This beast will convert an object and method into a vector
#define ObjectMethod(object, method) \
	([](void * o){ \
		static void * obj = o; \
		return [](){ \
			(static_cast<decltype(object)>(obj))->method(); \
			}; } \
	)(object)

#define Method(method) ObjectMethod(this, method)

#endif

#endif

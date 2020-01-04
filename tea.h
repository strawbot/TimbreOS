#include "clocks.h"
#include "project_defs.h"
#include "ttypes.h"
#include "dictionary.h"

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
    Long dueDate; // 1ms resolution; up to 48.5 days
    bool asap;
} TimeEvent;

// time base
Long get_uptime(); // seconds since startup; 136 year rollover
Long getTime(void);    // ms time stamp; 49 day rollover

// note use of 8 byte intermediate precision; accomodate range of values for ONE_SECOND
#define msec(t) ((Long)(((Octet)(t)*ONE_SECOND) / 1000))
#define secs(t) msec(t * 1000)
#define mins(t) secs(t*60)
#define hours(t) mins(t*60)
#define days(t) hours(t*24)
#define to_msec(n) ((Long)((Octet)(n)*1000/ONE_SECOND))
#define to_secs(n) (to_msec(n)/1000)

// time
void after(Long t, vector action);
void in(Long t, vector action);

// events
typedef vector Event[1];
void when(Event e, vector a);
void never(Event e);

// actions
#define now(action) actionRun(action)
void later(void (*a)());
void run();
void action_slice();

// measure execution time
#define namedAction(m) actor(m, #m)

void actor(vector action, const char * name);
void actionRun(vector m);
Cell * action_stat(vector m);

// inactions
void no_action();
void stop_te(vector v);
void stop_action(vector v);
void stop(vector v); // all places;

void init_tea();

// C++ support
#ifdef __cplusplus
}
// This beast will convert an object and method into a function vector
#define ObjectMethod(object, method) \
    ( [](decltype(object) o) { \
            static decltype(object) obj = o; \
            return [](){ obj->method(); }; \
        } \
    )(object)

#define Method(method) ObjectMethod(this, method)

#endif

#endif

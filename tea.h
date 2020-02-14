#ifndef TEA_H_
#define TEA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clocks.h"
#include "project_defs.h"
#include "ttypes.h"
#include "dictionary.h"

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

// event recorder
#ifndef N_EVENTS
#define N_EVENTS 500
#endif

#ifndef FIRST_EVENT
#define FIRST_EVENT (const char *)secs(3)
#endif
 
void record_event(const char * e);
void play_events();

#ifdef __cplusplus
}
// C++ support
class TeaCup {
    public:
    static const Byte cups = 3;
    Byte cup;
};

// possible workaround for multiple instances
// template with class and method for array of struct
// This beast will convert an object and method into an array of function vectors
#define OMVI(method, name) \
    ( [this](decltype(this) o) { \
            static decltype(this) obj[cups]; \
            obj[cup] = o; \
            vector lambs[cups] = { \
                []{ obj[0]->method(); }, \
                []{ obj[1]->method(); }, \
                []{ obj[2]->method(); } }; \
            actor(lambs[cup], name #method); \
            return lambs[cup]; \
        } \
    )(this)

#define ObjectMethodName(object, method, name) \
    ( [](decltype(object) o) { \
            static decltype(object) obj; \
            obj = o; \
            vector action = []{ obj->method(); }; \
            actor(action, name "->" #method); \
            return  action; \
        } \
    )(object)

#define ObjectMethod(object, method) ObjectMethodName(object, method, #object)
#define MethodName(method, name) OMVI(method, name)
#define MethodThis(method) OMVI(method, "this->")
#define GET_MACRO(_1,_2,NAME,...) NAME
#define Method(...) GET_MACRO(__VA_ARGS__, MethodName, MethodThis)(__VA_ARGS__)

#endif

#endif

#ifndef TEA_H_
#define TEA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clocks.h"
#include "project_defs.h"
#include "ttypes.h"
#ifndef	MINIMAL_TEA
#include "dictionary.h"
#endif

// resolve in application
#define BLACK_HOLE() system_failure()

void system_failure(void); // application defines system_failure();
// void system_failure() { while (true); } // DEBUGGING

#define safe(code) 	\
	ENTER_SAFE_REGION() \
	code; \
	LEAVE_SAFE_REGION()

#define outside(action) \
	pushq((Cell)action, outsideq), \
	critical_action()
// void critical_action() { set_critical_interrupt(true); }

// time
typedef struct TimeEvent {
    struct TimeEvent *next;
    vector action;
    Long dueDate; // 1ms resolution; up to 48.5 days
    bool asap;
} TimeEvent;

// time base
Long get_uptime(void); // seconds since startup; 136 year rollover
Long getTime(void);    // ms time stamp; 49 day rollover

// note use of 8 byte intermediate precision; accomodate range of values for ONE_SECOND
#define usec(t) ((Long)(((Octet)(t)*ONE_SECOND) / 1000000))
#define msec(t) ((Long)(((Octet)(t)*ONE_SECOND) / 1000))
#define secs(t) msec(t * 1000)
#define mins(t) secs(t*60)
#define hours(t) mins(t*60)
// #define days(t) hours(t*24)  - only 72 hrs available
#define to_msec(n) ((Long)((Octet)(n)*1000/ONE_SECOND))
#define to_secs(n) (n/ONE_SECOND)

// CLI
void ticks_ms(void);

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
int run(void);
void action_slice(void);

// measure execution time
#define namedAction(m) actor(m, #m)

void printActionName(Cell x);
void actor(vector action, const char * name);
void actionRun(vector m);
Cell * action_stat(vector m);

// inactions
void no_action(void);
void stop_te(vector v);
void stop_action(vector v);
void stop(vector v); // all places;

void init_tea(void);

#ifndef N_EVENTS
#define N_EVENTS 500
#endif

#ifndef FIRST_EVENT
#define FIRST_EVENT (const char *)secs(3)
#endif

#ifdef __GNUC__
    #define RE() record_event(__PRETTY_FUNCTION__)
#else
    #define RE() record_event(__func__)
#endif

#define RE1() record_event(FIRST_EVENT)

void record_event(const char * e);
void play_events(void);
void record_event_off(void);

#ifdef __cplusplus
}
// C++ support
class TeaCup {
    public:
    static const Byte cups = 3;
    Byte cup;
    const char * cupi() {
        switch(cup) {
            case 0: return "cup0";
            case 1: return "cup1";
            case 2: return "cup2";
            default: return "cupi";
        }
    }
};

#define CUP() record_event(cupi())

#define Timebox(time, event, action) \
    { static vector box_action = action; \
      static vector box = []{ never(event); stop(box); box_action(); }; \
      when(event, box); after(time, box); }

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

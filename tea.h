#ifndef TEA_H_
#define TEA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clocks.h"
#include "project_defs.h"
#include "ttypes.h"
#include "dictionary.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

// resolve in application
#define BLACK_HOLE(reason) system_failure(reason)
#define TE_LOOPED 1
#define TE_UNDERRUN 2
#define LATER_NULL 3
#define ACTION_OVERRUN 4
#define ACTION_NULL 5
#define SFP_UNDERRUN 6
#define IN_0 7
#define TE_MISSING 8
#define FS_FAILURE 9

void system_failure(Long reason); // application defines system_failure();
// void system_failure() { while (true); } // DEBUGGING
// Note: safe() will fail for comma separated statements, use ;
#define safe(code) 	\
	ENTER_SAFE_REGION() \
	code \
	LEAVE_SAFE_REGION()

// time
typedef struct TimeEvent {
    struct TimeEvent *next;
    vector action;
    Long dueDate; // 1ms resolution; up to 48.5 days
    bool asap;
} TimeEvent;

// time base
uint32_t getUptime();
Long getTime(void);    // ms time stamp; 49 day rollover

// interval length of time before rollover is determined by the number of bits used for precision
// for ONE_SECOND defined as 16384 ticks, this is 61us ticks with a maximum 32-14=18 bits of seconds
//  18 bits of seconnds is 72.8h
//  17 bits of seconds is 36.4h; more than a day; 15 bits for seconds or 32768 ticks or 30.5 us res
//  16 bits of seconds is 18.2h
// note use of 8 byte intermediate precision; accomodate range of values for ONE_SECOND
#define usec(t) ((Long)(((Octet)(t)*ONE_SECOND) / 1000000))
#define msec(t) ((Long)(((Octet)(t)*ONE_SECOND) / 1000))
#define secs(t) msec((t) * 1000)
#define mins(t) secs((t)*60)
#define hours(t) mins((t)*60)
// #define days(t) hours(t*24)  - only 72 hrs available
#define to_usec(n) ((Long)((Octet)(n)*1000000/ONE_SECOND))
#define to_msec(n) ((Long)((Octet)(n)*1000/ONE_SECOND))
#define to_secs(n) ((n)/ONE_SECOND)

// CLI
void ticks_ms();

// time — t MUST be expressed with a converter: usec(), msec(), secs(), mins().
// Never pass a raw integer literal; the unit depends on ONE_SECOND which varies by board.
void after(Long t, vector action);
void in(Long t, vector action);

// events
extern Event alarmEvent;
void when(Event e, vector a);
void never(Event e);

// actions
#define now(action) actionRun(action)
void later(void (*a)());
void run();
void action_slice();

// measure execution time
#define namedAction(m) actor(m, #m)

void printActionName(Cell x);
void actor(vector action, const char * name);
void actionRun(vector m);
Cell * action_stat(vector m);

// inactions
void no_action();
void stop_te(vector v);
void stop_action(vector v);
void stop(vector v); // all places;

void init_tea();

#ifndef N_EVENTS
#define N_EVENTS 500
#endif

#ifndef FIRST_EVENT
#define FIRST_EVENT (const char *)secs(3)
#endif

#ifdef __GNUC__
    #define RE() record_event(__PRETTY_FUNCTION__)
    #define RI() record_interrupt(__PRETTY_FUNCTION__)
#else
    #define RE() record_event(__func__)
    #define RI() record_interrupt(__func__)
#endif

#define RE1() record_event(FIRST_EVENT)
// handy search and replace:  s/^([a-zA-Z][^)]+\) \{)/$1 RE();/

void record_event(const char * e);
void play_events();
void record_event_off();
void record_interrupt(const char * e); // only record e if in an interrupt

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
            static_assert(cups == 3, "OMVI lambda array must match cups — add/remove a slot here"); \
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

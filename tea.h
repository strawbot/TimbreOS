#include "ttypes.h"
#include "clocks.h"
#include "project_defs.h"

#ifndef TEA_H_
#define TEA_H_

#define BLACK_HOLE()	while(true) ; // DEBUGGING

// time
typedef struct TimeEvent {
	struct TimeEvent * next;
	vector action;
	Short dueDate; // up to 65000 ms
} TimeEvent;

// time base
Long get_uptime();  // seconds since startup
Long getTime();		// ms time stamp; 49 day rollover

#define msec(t) ((Long)(((Octet)(t) * ONE_SECOND) / 1000))
#define secs(t) msec(t*1000)
#define to_msec(n) (to_secs((n)*1000))
#define to_secs(n) ((n)/ONE_SECOND)

// Events
// Event test;
// when(test, act1);
// if (immediate)  now(*test);  else  later(*test);
// never(test);
typedef vector Event[1];

void when(Event e, vector a);
void never(Event e);
void after(Long t, vector action);

// actions
#define now(action) (action)()
void later(vector a);
void run();
void no_action();

void init_tea();

#endif

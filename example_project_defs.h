#ifndef PROJECT_DEFS_H_
#define PROJECT_DEFS_H_

// 14 MHz clock ticks; wraps after 5 minutes; 71.4 ns resolution
#define CLOCK_MHZ 14
#define getTicks() 0

#error blah

#define safe(code) 	code

#define NUM_ACTIONS 40
#define NUM_TE 40

#define N_EVENTS 100
#define FIRST_EVENT (const char *)secs(5)

#endif

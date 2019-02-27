#ifndef _EVENT_H_
#define _EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "queue.h"
#ifdef __cplusplus
}
#endif

#define Event(n,name)  NEW_Q(n+1, name) = {QDATA+n+1,QDATA,QDATA+n+1, (Cell)noaction}

#define extEvent(event) extern Cell event[]

void noaction(); // queue sentinel separates repeaters and one shots

// void when(Cell * event, vector action);
void once(Cell * event, vector action);
void happen(Cell * event);
void clearEvent(Cell * event);
void stopEventAction(Cell * eventq, vector action);

// #ifdef __cplusplus
// }

#include "EventQueue.h"

// void when(EventQueue * event, vector action);
// void when(EventQueue * event, void *cpp_obj, void *cpp_method);

// void once(EventQueue * event, vector action);
// void once(EventQueue * event, void *cpp_obj, void *cpp_method);
//
// void happen(EventQueue * event);

// #else
// void when(Cell * event, vector action);


#endif

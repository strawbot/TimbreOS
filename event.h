#include "queue.h"

#ifndef _EVENT_H_
#define _EVENT_H_

#define Event(n,name)  NEW_Q(n+1, name) = {QDATA+n+1,QDATA,QDATA+n+1, (Cell)noaction}

void when(Cell * event, vector action);
void once(Cell * event, vector action);
void happen(Cell * event);
void setEvent(Cell * event);

void noaction();

#endif

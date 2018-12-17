#include "queue.h"

#ifndef _EVENT_H_
#define _EVENT_H_

#define Event(n,name)  QUEUE(n+1, name)

void when(Cell * event, vector action);
void once(Cell * event, vector action);
void happen(Cell * event);
void setEvent(Cell * event);

#endif

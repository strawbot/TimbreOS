#include "ttypes.h"
#include "clocks.h"
#include "project_defs.h"

#ifndef TEA_H_
#define TEA_H_

#define BLACK_HOLE()	while(true) ; // DEBUGGING

#define now(action) (action)()
void later(vector a);

typedef vector Event[1];

void when(Event e, vector a);
void never(Event e);

void after(Short t, vector action);

void run();
void init_tea();

typedef struct TimeEvent {
	struct TimeEvent * next;
	vector action;
	Short dueDate;
}TimeEvent;

#endif

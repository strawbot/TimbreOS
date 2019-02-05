// Events can be awaited by multiple actions
/*
	  when(some_event, some_action);

	for events, they have a local store which gets loaded up with machines that
	subscribe to the event. When the event happens, all the loaded machines get run.
	One way to do that is to dump all the machines into the actionq and leave. This
	is desireable as it follows the machine philosophy of "write shallow functions"
	and "run now, next or later"
	Whether to remember those machines or not is guided by:
	"action must be taken by the running program to be run another action"

	Key points of definition:
	 - repeat or not
	 	if repeat then with choice or not
	   * without repeat, events may be missed. given a choice, for oneof, some(n,)
	     or all

	 whether to repeat call a function for an event multiple times
	 needing only one event is a reasonable expectation of the API
	 as is continued but with unsubscribe

	 create an event structure that has an action for: subscribing, unsubscribing, happened
	 API:
	   Event(10, someevent);

	   when(event, action);
	   once(event, action);
	   never(event, action);

	   could use a queue for when and once. Plant an end of queue sentinal in the queue (increase size by 1 at compile time) the p end of the queue is persistant and is used by when, q is quiet and is used by once
		    whens pSq onces
		push, pop     stuff, pull

*/

#include "machines.h"
#include "printers.h"

void noaction() {}

// event connections
void clearEvent(Cell * eventq) {
	zeroq(eventq);
	pushq((Cell)noaction, eventq);
}

void when(Cell * eventq, vector action) {
    if (leftq(eventq))
        pushq((Cell)action, eventq);
    else {
    	char * name;
        print("\nError: eventq full!  ");
        if ((name = getMachineName((Cell)action)) != NULL)
            print("Trying to eventize:"), print(name);
    }
}

void once(Cell * eventq, vector action) {
    if (leftq(eventq))
        stuffq((Cell)action, eventq);
    else {
    	char * name;
        print("\nError: eventq full!  ");
        if ((name = getMachineName((Cell)action)) != NULL)
            print("Trying to eventize:"), print(name);
    }
}

// event occurrance
static void do_once(Cell * eventq) {
	while(q(eventq) != (Cell)noaction) { next((vector)pullq(eventq)); }
}

static void do_every(Cell * eventq) {
	while(p(eventq) != (Cell)noaction) {
		next((vector)p(eventq)); stuffq(popq(eventq),eventq);
	}
	stuffq(popq(eventq),eventq);
}

void happen(Cell * eventq) {
	do_once(eventq);
	do_every(eventq);
}

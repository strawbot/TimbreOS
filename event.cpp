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
#include "event.h"

extern "C" {

void whenEventQ(struct EventQueue_t* event, void* cpp_obj, void* cpp_method) {
  static_cast<EventQueue*>(event)->push(cpp_obj, cpp_method, 1);
}

void onceEventQ(struct EventQueue_t* event, void* cpp_obj, void* cpp_method) {
  static_cast<EventQueue*>(event)->push(cpp_obj, cpp_method, 0);
}

void stopEventQ(struct EventQueue_t* event, void* cpp_obj, void* cpp_method) {
  static_cast<EventQueue*>(event)->remove(cpp_obj, cpp_method);
}

void never(struct EventQueue_t* event) {
  static_cast<EventQueue*>(event)->clear();
}

void happen(struct EventQueue_t* event) {
  static_cast<EventQueue*>(event)->happen();
}
}

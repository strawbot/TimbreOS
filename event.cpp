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
	   Event(10, someevent);	// define
	   extEvent(event);			// declare

	   once(event, action);		// oneshot
	   when(event, action);		// connect
	   never(event, action);	// nothing
	   stop(event, action);		// prevent

	   happen(event);			// moment

*/
#include "event.h"

extern "C" {

void whenEvent(struct EventQueue_t* event, void* cpp_obj, void* cpp_method) {
    queue_type e = {cpp_obj, cpp_method, 1};
  static_cast<EventQueue*>(event)->push(&e);
}

void onceEvent(struct EventQueue_t* event, void* cpp_obj, void* cpp_method) {
    queue_type e = {cpp_obj, cpp_method, 0};
  static_cast<EventQueue*>(event)->push(&e);
}

void stopEvent(struct EventQueue_t* event, void* cpp_obj, void* cpp_method) {
  static_cast<EventQueue*>(event)->remove(cpp_obj, cpp_method);
}

void never(struct EventQueue_t* event) {
  static_cast<EventQueue*>(event)->clear();
}

void happen(struct EventQueue_t* event) {
  static_cast<EventQueue*>(event)->happen();
}
}

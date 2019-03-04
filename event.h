/*
 * File:   events.h
 * Author: paul
 *
 * Created on February 26, 2019, 6:54 AM
 */
// Events can be awaited by multiple actions

/*
      when(some_event, some_action);

    for events, they have a local store which gets loaded up with machines that
    subscribe to the event. When the event happens, all the loaded machines get run.

     API:
       Event(10, someevent);	// define
       extEvent(event);			// declare

       once(event, action);		// oneshot
       when(event, action);		// connect
       never(event, action);	// nothing
       stop(event, action);		// prevent

       happen(event);			// moment

*/

#ifndef EVENTS_H
#define EVENTS_H

#include <stdio.h>
#include <string.h>

// structures common to C and C++
typedef struct Action {
  void* object, *method;
  char persist;
}Action;

typedef struct EventQueue {
  unsigned _size, _head, _tail;
  Action * _queue;
} EventQueue;

#ifdef __cplusplus

class EventQueueClass : public EventQueue {
 public:
  int count() { return (_head - _tail) % _size; }

  void clear() { _head = _tail = 0; }

  void push(Action* entry) {
    memcpy(&_queue[_head++], entry, sizeof(Action));
    _head %= _size;
  }

  void pop(Action* entry) {
    memcpy(entry, &_queue[_tail++], sizeof(Action));
    _tail %= _size;
  }

  void remove(void* cpp_obj, void* cpp_method) {
    for (int i = 0, total = count(); i < total; i++) {
      Action e;

      pop(&e);

      if (e.object != cpp_obj && e.method != cpp_method) {
        push(&e);
      }
    }
  }

  void happen() {
    int total = count();

    for (int i = 0; i < total; i++) {
      Action e;

      pop(&e);
      if (e.persist)
        push(&e);

      if (e.object == nullptr) {
        void (*callback)() = (void (*)())e.method;
        callback();
      } else {
        void (*method)(void*) = (void (*)(void*))e.method;
        method(e.object);
      }
    }
  }

};

extern "C" {
#endif

#define Event(e) Eventi(3, e)

#define Eventi(size, e)              \
  Action e##qt[size + 1]; \
  EventQueue e[1] = {{size + 1, 0, 0, e##qt}}

#define extEvent(event) extern EventQueue event[]

void onceEvent(EventQueue* event, void* cpp_obj, void* cpp_method);
void whenEvent(EventQueue* event, void* cpp_obj, void* cpp_method);
void stopEvent(EventQueue* event, void* cpp_obj, void* cpp_method);

void never(EventQueue* event);
void happen(EventQueue* event);

#ifdef __cplusplus
// C++ in C
}
// C++
void once(EventQueue* event, void* cpp_obj, void* cpp_method); // possible sol'n for c++ functions; c++ uses 2 for function vector and 3 for object method
#define once(e, h) onceEvent(e, (void*) this, (void*)&h)
#define when(e, h) whenEvent(e, (void*) this, (void*)&h)
#define stop(e, h) stopEvent(e, (void*) this, (void*)&h)

#else

// C
#define once(e, h) onceEvent(e, NULL, (void*)(Cell)&h)
#define when(e, h) whenEvent(e, NULL, (void*)(Cell)&h)
#define stop(e, h) stopEvent(e, NULL, (void*)(Cell)&h)

#endif

#endif /* EVENTS_H */

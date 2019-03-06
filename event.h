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
	subscribe to the event. When the event happens, all the loaded machines get
   run.

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
typedef void (*unafun)(void*); // unary function


// structures common to C and C++
typedef struct Action {
    void *object, *method;
    char persist;
} Action;

typedef struct EventQueue {
    unsigned _size, _head, _tail;
    Action* _queue;
} EventQueue;

#ifdef __cplusplus
extern "C++" void next(void* object, unafun unary);

class EventQueueClass : public EventQueue {
public:
    unsigned count() { return (_head - _tail) % _size; }

    void clear() { _head = _tail = 0; }

    void push(Action* entry) {
        memcpy(&_queue[_head++], entry, sizeof(Action));
        _head %= _size;
    }

    void pop(Action* entry) {
        memcpy(entry, &_queue[_tail++], sizeof(Action));
        _tail %= _size;
    }

    void remove(void* cpp_obj, unafun cpp_method) {
        for (unsigned i = 0, total = count(); i < total; i++) {
            Action e;

            pop(&e);

            if (e.object != cpp_obj && e.method != cpp_method) {
                push(&e);
            }
        }
    }

    void happen() {
        unsigned total = count();

        for (unsigned i = 0; i < total; i++) {
            Action e;

            pop(&e);
            if (e.persist)
                push(&e);

            next(e.object, (unafun)e.method);
            // void (*method)(void*) = reinterpret_cast<void (*)(void*)>(e.method);
            // method(e.object);
        }
    }
};

extern "C" {
#endif

#include "ttypes.h"

void jump(void* object);

#define Event(e) Eventi(3, e)

#define Eventi(size, e)     \
    Action e##qt[size + 1]; \
    EventQueue e[1] = { { size + 1, 0, 0, e##qt } }

#define extEvent(event) extern EventQueue event[]

void never(EventQueue* event);
void happen(EventQueue* event);

#ifdef __cplusplus
// C++ in C
}
// C++
extern "C"   void once(EventQueue* event, vector action);
extern "C++" void once(EventQueue* event, void* cpp_obj, unafun cpp_method);
extern "C"   void when(EventQueue* event, vector action);
extern "C++" void when(EventQueue* event, void* cpp_obj, unafun cpp_method);
extern "C"   void stopEvent(EventQueue* event, vector action);
extern "C++" void stopEvent(EventQueue* event, void* cpp_obj, unafun cpp_method);


#else

// C
void once(EventQueue* event, vector action);
void when(EventQueue* event, vector action);
void stopEvent(EventQueue* event, vector action);

#endif

#endif /* EVENTS_H */

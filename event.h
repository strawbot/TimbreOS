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
       Eventi(10, someevent);	// define
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
#include "ttypes.h"


// structures common to C and C++
typedef struct Action {
    const char * name;
    void *object, *method;
    char persist;
    bool active;
} Action;

typedef struct EventQueue {
    const char * name;
    Byte _size, _head, _tail;
    Action* _queue;
} EventQueue;

#ifdef __cplusplus
extern "C++" void next(void* object, unafun unary, const char * name = "");

extern "C" {

void next(vector object);
void jump(void* object);
void print(const char *);
void printTimeDate();
}

class EventQueueClass : public EventQueue {
public:
    Byte count() {
        ATOMIC_SECTION_ENTER;
        Byte c = (_head >= _tail) ? (_head - _tail) : (_size - (_tail - _head));
        ATOMIC_SECTION_LEAVE;
        return c;
    }

    void clear() { _head = _tail = 0; }
    
    void push(Action* entry) {
        ATOMIC_SECTION_ENTER;
        if (count() < _size - 1) {
          memcpy(&_queue[_head++], entry, sizeof(Action));
          _head %= _size;
        } else {
          print("\nERROR: eventq full!"), printTimeDate();
          while (true);
        }
        ATOMIC_SECTION_LEAVE;
    }

    void pop(Action* entry) {
        ATOMIC_SECTION_ENTER;
        memcpy(entry, &_queue[_tail++], sizeof(Action));
        _tail %= _size;
        ATOMIC_SECTION_LEAVE;
    }
    
    void remove(void* cpp_obj, unafun cpp_method) {
        for (Byte i = 0, total = count(); i < total; i++) {
            Action e;

            pop(&e);

            if (e.object != cpp_obj && e.method != cpp_method) {
                push(&e);
            }
        }
    }

    void happen() {
        Byte total = count();

        for (Byte i = 0; i < total; i++) {
            Action e;

            pop(&e);
            if (e.persist)
                push(&e);

            if (e.method == jump)
                next((vector)e.object);
            else
                next(e.object, (unafun)e.method);
        }
    }
};

extern "C" {
#endif

// class/action - within same class
// class/object/action - calling methods from other classes, object needed

#define Event(e) Eventi(5, e)

#define ObjectMethod(object, method) object, [](void * o){ (static_cast<decltype(object)>(o))->method(); }
#define OM(o,m) ObjectMethod(o,m)
#define Method(method) OM(this, method)

#define Eventi(size, e)     \
    Action e##qt[size + 1]; \
    EventQueue e[1] = { { #e, size + 1, 0, 0, e##qt } }

#define extEvent(event) extern EventQueue event[]

void never(EventQueue* event); // need: never(EventQueue* event, vector action); and unafun 2
void happen(EventQueue* event);
void once(EventQueue* event, vector action);
void when(EventQueue* event, vector action);
void stopEvent(EventQueue* event, vector action);

#ifdef __cplusplus
}

extern "C++" {
void once(EventQueue* event, void* cpp_obj, unafun cpp_method, const char * name = "");
void when(EventQueue* event, void* cpp_obj, unafun cpp_method, const char * name = "");
void stopEvent(EventQueue* event, void* cpp_obj, unafun cpp_method);
}

#endif

#endif /* EVENTS_H */

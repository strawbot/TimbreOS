/*
 * File:   events.h
 * Author: paul
 *
 * Created on February 26, 2019, 6:54 AM
 */

#ifndef EVENTS_H
#define EVENTS_H

#include <stdio.h>
#include <string.h>

// structures common to C and C++
struct queue_type {
  void* object, *method;
  char persist;
};

struct EventQueue_t {
  unsigned _size, _head, _tail;
  struct queue_type* _queue;
};

#ifdef __cplusplus

class EventQueue : public EventQueue_t {
 public:
  int count() { return (_head - _tail) % _size; }

  void clear() { _head = _tail = 0; }

  void push(queue_type* entry) {
    memcpy(&_queue[_head++], entry, sizeof(queue_type));
    _head %= _size;
  }

  void pop(queue_type* entry) {
    memcpy(entry, &_queue[_tail++], sizeof(queue_type));
    _tail %= _size;
  }

  void happen() {
    int total = count();

    for (int i = 0; i < total; i++) {
      queue_type e;

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

  void remove(void* cpp_obj, void* cpp_method) {
    int total = count();

    for (int i = 0; i < total; i++) {
      queue_type e;

      pop(&e);

      if (e.object != cpp_obj && e.method != cpp_method) {
        push(&e);
      }
    }
  }
};

extern "C" {
#endif

#define Event(e) Eventi(3, e)

#define Eventi(size, e)              \
  struct queue_type e##qt[size + 1]; \
  struct EventQueue_t e[1] = {{size + 1, 0, 0, e##qt}}

void onceEvent(struct EventQueue_t* event, void* cpp_obj, void* cpp_method);
void whenEvent(struct EventQueue_t* event, void* cpp_obj, void* cpp_method);
void stopEvent(struct EventQueue_t* event, void* cpp_obj, void* cpp_method);

void never(struct EventQueue_t* event);
void happen(struct EventQueue_t* event);

#ifdef __cplusplus
// C++ in C
}
// C++
#define once(e, h) onceEvent(e, (void*) this, (void*)&h)
#define when(e, h) whenEvent(e, (void*) this, (void*)&h)
#define stop(e, h) stopEvent(e, (void*) this, (void*)&h)

#else

// C
#define once(e, h) onceEvent(e, NULL, (void*)(Cell)&h)
#define when(e, h) whenEvent(e, NULL, (void*)(Cell)&h)
#define stop(e, h) stopEvent(e, NULL, (void*)(Cell)&h)

#endif

#define extEvent(event) extern struct EventQueue_t event[]

#endif /* EVENTS_H */

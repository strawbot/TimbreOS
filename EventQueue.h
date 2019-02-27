/*
 * File:   events.h
 * Author: paul
 *
 * Created on February 26, 2019, 6:54 AM
 */

#ifndef EVENTS_H
#define EVENTS_H

#include <string.h>

struct queue_type {
    void *object, *method;
    char persist;
};

#ifdef __cplusplus

class EventQueue {
public:
    EventQueue(int size, queue_type *queue)
    {
        _size = size;
        _queue = queue;
        _head = 0;
        _tail = 0;
    }

    ~EventQueue() { }

    void push(void *cpp_obj, void *cpp_method, char persist)
    {
        _queue[_head++]={cpp_obj, cpp_method, persist};
        _head %= _size;
    }

    void push(void *c_handler, char persist)
    {
        _queue[_head++]={nullptr, c_handler, persist};
        _head %= _size;
    }

    int stop(void *cpp_obj, void *cpp_method)
    {
        int total = count();

        for (int i=0; i<total; i++)
        {
            queue_type e;

            if (pop(&e) < 0) return -1;

            if (e.object != cpp_obj && e.method != cpp_method)
            {
                push(e.object, e.method, e.persist);
            }
        }
        return 0;
    }

    int stop(void *c_handler)
    {
        int total = count();

        for (int i=0; i<total; i++)
        {
            queue_type e;

            if (pop(&e) < 0) return -1;

            if (e.method != c_handler)
            {
                push(e.method, e.persist);
            }
        }
        return 0;
    }

    void happen()
    {
        int total = count();

        for (int i=0; i<total; i++)
        {
            queue_type e;

            if (pop(&e) == 0)
            {
                if (e.object == nullptr)
                {
                    void (*callback)() = (void (*)()) e.method;
                    callback();
                }
                else
                {
                    void (*method)(void *) = (void (*)(void *)) e.method;
                    method(e.object);
                }
                if (e.persist)
                {
                    push(e.object, e.method, e.persist);
                }
            }
        }
    }

    int pop(queue_type *entry)
    {
        if (_head == _tail) return -1;

        memcpy(entry, &_queue[_tail], sizeof(queue_type));
        _tail = (_tail + 1) % _size;
        return 0;
    }

    int count()
    {
        return (_head - _tail) % _size;
    }

    void clear()
    {
        _head = _tail = 0;
    }

private:
    int _size, _head, _tail;
    queue_type *_queue;
};

#define whenEventQCpp(e, c, o, m) _whenEventQ(e, (void *) &o, (void *) &c::m)

void _whenEventQ(EventQueue *event, void *cpp_obj, void *cpp_method);
void onceEventQ(EventQueue *event, void *cpp_obj, void *cpp_method);
void stopEventQ(EventQueue *event, void *cpp_obj, void *cpp_method);

// outside a class
#define EventQ(n,name) queue_type name##queue[n]; EventQueue __##name(n, name##queue); EventQueue *name = &__##name

// inside a class: for the privates
#define EQ(size, name) \
	queue_type name##queue[size]; \
	EventQueue __##name; \
	EventQueue *name = &__##name

// : for the constructor
#define EQinit(name) __##name(sizeof(name##queue), name##queue)

#define EQwhenever(event, action, persist) \
        event->push((void *) this, (void *) &action, persist)
#define EQwhen(event, action) EQwhenever(event, action, 1)
#define EQonce(event, action) EQwhenever(event, action, 0)

#else

// #define Event  "You can only instantiate an event in C++"
typedef struct { int _size, _head, _tail; } EventQueue;

#endif

#ifdef __cplusplus
extern "C"
{
#endif
    // void whenEventQ(EventQueue *event, void (*c_handler)());
    // void onceEventQ(EventQueue *event, void (*c_handler)());
    // void stopEventQ(EventQueue *event, void (*c_handler)());
    // void clearEventQ(EventQueue *event);
    // void happenEventQ(EventQueue *event);
#ifdef __cplusplus
}
#endif

#endif /* EVENTS_H */

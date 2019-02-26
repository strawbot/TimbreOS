#include "EventQueue.h"

void _whenEventQ(EventQueue *event, void *cpp_obj, void *cpp_method)
{
    event->push(cpp_obj, cpp_method, 1);
}

void onceEventQ(EventQueue *event, void *cpp_obj, void *cpp_method)
{
    event->push(cpp_obj, cpp_method, 0);
}

void stopEventQ(EventQueue *event, void *cpp_obj, void *cpp_method)
{
    event->stop(cpp_obj, cpp_method);
}

extern "C"
{
    void whenEventQ(EventQueue *event, void (*c_handler)())
    {
        event->push((void *) c_handler, 1);
    }
    
    void onceEventQ(EventQueue *event, void (*c_handler)())
    {
        event->push((void *) c_handler, 0);
    }
    
    void stopEventQ(EventQueue *event, void (*c_handler)())
    {
        event->stop((void *) c_handler);
    }
    
    void clearEventQ(EventQueue *event)
    {
        event->clear();
    }
    
    void happenEventQ(EventQueue *event)
    {
        event->happen();
    }
}

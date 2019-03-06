#include "event.h"

void once(EventQueue* event, void* cpp_obj, unafun cpp_method) {
    Action e = { cpp_obj, (void *)cpp_method, 0 };
    static_cast<EventQueueClass*>(event)->push(&e);
}

void when(EventQueue* event, void* cpp_obj, unafun cpp_method) {
    Action e = { cpp_obj, (void *)cpp_method, 1 };
    static_cast<EventQueueClass*>(event)->push(&e);
}

void stopEvent(EventQueue* event, void* cpp_obj, unafun cpp_method) {
    static_cast<EventQueueClass*>(event)->remove(cpp_obj, cpp_method);
}

extern "C" {

void once(EventQueue* event, vector action) {
    once(event, (void*)action, jump);
}

void when(EventQueue* event, vector action) {
    when(event, (void*)action, jump);
}

void stopEvent(EventQueue* event, vector action) {
    static_cast<EventQueueClass*>(event)->remove((void *)action, jump);
}

void never(EventQueue* event) {
    static_cast<EventQueueClass*>(event)->clear();
}

void happen(EventQueue* event) {
    static_cast<EventQueueClass*>(event)->happen();
}

#include <cstdint>
void jump(void* object) { // with function pointer as object use this method
    (reinterpret_cast<void (*)()>(object))();
}
}

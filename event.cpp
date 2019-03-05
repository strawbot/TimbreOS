#include "event.h"

extern "C" {

void whenEvent(EventQueue* event, void* cpp_obj, void* cpp_method) {
    Action e = { cpp_obj, cpp_method, 1 };
    static_cast<EventQueueClass*>(event)->push(&e);
}

void onceEvent(EventQueue* event, void* cpp_obj, void* cpp_method) {
    Action e = { cpp_obj, cpp_method, 0 };
    static_cast<EventQueueClass*>(event)->push(&e);
}

//void once(EventQueue* event, void* cpp_obj, void* cpp_method) {
//    onceEvent(event, cpp_obj, cpp_method);
//}

void stopEvent(EventQueue* event, void* cpp_obj, void* cpp_method) {
    static_cast<EventQueueClass*>(event)->remove(cpp_obj, cpp_method);
}

void never(EventQueue* event) {
    static_cast<EventQueueClass*>(event)->clear();
}

void happen(EventQueue* event) {
    static_cast<EventQueueClass*>(event)->happen();
}

#include <cstdint>
void jump(void* object) { // with function pointer as object use this method
    printf("\nJummping to :%lX", reinterpret_cast<uintptr_t>(object));
    (reinterpret_cast<void (*)()>(object))();
}
}

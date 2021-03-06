#include "event.h"
#include "machines.h"

void once(EventQueue* event, void* cpp_obj, unafun cpp_method, const char * name) {
    Action e = {.name=name, .object=(void *)cpp_obj, .method=(void *)cpp_method, .persist=0, .active=0 };
    ATOMIC_SECTION_ENTER;
    static_cast<EventQueueClass*>(event)->pushOnce(&e);
    ATOMIC_SECTION_LEAVE;
}

void when(EventQueue* event, void* cpp_obj, unafun cpp_method, const char * name) {
    Action e = {.name=name, .object=(void *)cpp_obj, .method=(void *)cpp_method, .persist=1, .active=0 };
    ATOMIC_SECTION_ENTER;
    static_cast<EventQueueClass*>(event)->pushOnce(&e);
    ATOMIC_SECTION_LEAVE;
}

void stopEvent(EventQueue* event, void* cpp_obj, unafun cpp_method) {
    ATOMIC_SECTION_ENTER;
    static_cast<EventQueueClass*>(event)->remove(cpp_obj, cpp_method);
    ATOMIC_SECTION_LEAVE;
}

extern "C" {

void once(EventQueue* event, vector action) {
    once(event, (void*)action, jump);
}

void when(EventQueue* event, vector action) {
    when(event, (void*)action, jump);
}

void stopEvent(EventQueue* event, vector action) {
    ATOMIC_SECTION_ENTER;
    static_cast<EventQueueClass*>(event)->remove((void *)action, jump);
    ATOMIC_SECTION_LEAVE;
}

void never(EventQueue* event) {
    ATOMIC_SECTION_ENTER;
    static_cast<EventQueueClass*>(event)->clear();
    ATOMIC_SECTION_LEAVE;
}

void only(EventQueue* event, vector action) {
    never(event);
    once(event, (void*)action, jump);
}

void happen(EventQueue* event) {
    static_cast<EventQueueClass*>(event)->happen();
}

void now(EventQueue* event) {
    static_cast<EventQueueClass*>(event)->now();
}

#include <cstdint>
void jump(void* object) { // with function pointer as object use this method
    (reinterpret_cast<vector>(object))();
}
}

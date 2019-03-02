#include "event.h"

extern "C" {

void whenEvent(struct EventQueue* event, void* cpp_obj, void* cpp_method) {
    queue_type e = {cpp_obj, cpp_method, 1};
  static_cast<EventQueueClass*>(event)->push(&e);
}

void onceEvent(struct EventQueue* event, void* cpp_obj, void* cpp_method) {
    queue_type e = {cpp_obj, cpp_method, 0};
  static_cast<EventQueueClass*>(event)->push(&e);
}

void stopEvent(struct EventQueue* event, void* cpp_obj, void* cpp_method) {
  static_cast<EventQueueClass*>(event)->remove(cpp_obj, cpp_method);
}

void never(struct EventQueue* event) {
  static_cast<EventQueueClass*>(event)->clear();
}

void happen(struct EventQueue* event) {
  static_cast<EventQueueClass*>(event)->happen();
}
}

#include "EventQueue.h"

void when(EventQueue * event, void * action);
void when(EventQueue * event, void *cpp_obj, void *cpp_method);

void once(EventQueue * event, void *action);
void once(EventQueue * event, void *cpp_obj, void *cpp_method);

void happen(EventQueue * event);

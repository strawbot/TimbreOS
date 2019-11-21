#include "clocks.h"
#include "tea.h"
#include "queue.h"
#include "em_core.h"

// Time
#define NUM_TE 20

static TimeEvent tes[NUM_TE], te_done, te_todo; // time events and todo and done lists

static void te_return(TimeEvent* te) {
	te->next = te_done.next;
	te_done.next = te;
}

static TimeEvent* te_borrow() {
	TimeEvent* te = te_done.next;
	if (te) te_done.next = te->next;
	te->next = NULL;
	return te;
}

static void add_dueDate(TimeEvent* te) {
	TimeEvent * te_next, * te_curr = &te_todo;
	Short last_dueDate = get_last_dueDate();
	Short delta = last_dueDate - te->dueDate;

	while ((te_next = te_curr->next) != NULL) {
		Short width = last_dueDate - te_next->dueDate;
		if (delta < width)
			break;
		te_curr = te_next;
	}
	te->next = te_next;
	te_curr->next = te;
	set_dueDate(te_todo.next->dueDate);
}

void after(Short t, vector action) {
	CORE_ATOMIC_SECTION(
	TimeEvent* te = te_borrow();
	if (te) {
		te->dueDate = get_dueDate(t);
		te->action = action;
		add_dueDate(te);
	} else BLACK_HOLE();)
}

void check_dueDates(Short last_dueDate) {
	while (te_todo.next) {
		if (te_todo.next->dueDate != last_dueDate) {
			set_dueDate(te_todo.next->dueDate);
			return;
		}

		TimeEvent* te = te_todo.next;
		te_todo.next = te->next;
		vector action = te->action;
		te_return(te);
		action();
	}
}

static void init_time() {
	te_todo.next = te_done.next = NULL;

	for (Byte i = 0; i < NUM_TE; i++) {
		te_return(&tes[i]);
	}
}

// Events
// Event test;
// when(test, act1);
// if (immediate)  now(*test);  else  later(*test);
// never(test);

typedef vector Event[1];

void when(Event e, vector a) { *e = a; }
static void no_action() {}
void never(Event e) { when(e, no_action); }

// Actions
static QUEUE(20, actionq);

void later(vector a) { CORE_ATOMIC_SECTION( pushq((Cell)a, actionq);) }

void run() {
	while (queryq(actionq)) {
		vector action = (vector)pullq(actionq);
		action();
	}
}

void stop_te(vector v) {
	CORE_ATOMIC_SECTION(
	TimeEvent* te = te_todo.next;
	while (te) {
		if (te->action == v)  te->action = no_action;
		te = te->next;
	})
}

void stop_action(vector v) {
	CORE_ATOMIC_SECTION(
	for (Long n = queryq(actionq); n; n--) {
		Cell a = pullq(actionq);
		if (a != (Cell)v)  pushq(a, actionq); // develop iter q tool to read and write without moving items
	})
}

void stop(vector v) {
	stop_te(v);
	stop_action(v);
}

void init_tea() {
	zeroq(actionq);
	init_time();
	init_clocks();
}

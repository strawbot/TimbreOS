#include "clocks.h"
#include "tea.h"
#include "queue.h"
#include "em_core.h"

// 16 bit time tracker; ms and S
static Long uptime = 0;
static Short last_dueDate, zero_ms; // points on the number wheel

static void one_second() {
	uptime++;
	zero_ms = last_dueDate;
	after(secs(1), one_second);
}

Long getTime() { 
	Short ms = to_msec(zero_ms - get_dueDate(0));
	return uptime*1000 + ms;
}

// Time Events
#define NUM_TE 40

static TimeEvent tes[NUM_TE], te_done, te_todo; // time events and todo and done lists

static void te_return(TimeEvent* te) {
	te->next = te_done.next;
	te_done.next = te;
}

static TimeEvent* te_borrow() {
	TimeEvent* te = te_done.next;
	if (te) {
		te_done.next = te->next;
		te->next = NULL;	
	} 
	return te;
}

static void schedule_te(TimeEvent* te) {
	TimeEvent * te_next, * te_curr = &te_todo;
	Short ref = last_dueDate; // time reference
	Short delta = ref - te->dueDate;

	while ((te_next = te_curr->next) != NULL) {
		Short width = ref - te_next->dueDate;
		if (delta < width)
			break;
		te_curr = te_next;
	}
	te->next = te_next;
	te_curr->next = te;
	set_dueDate(te_todo.next->dueDate);
}

void after(Long t, vector action) {
	CORE_ATOMIC_SECTION(
	TimeEvent* te = te_borrow();
	if (te == NULL)  BLACK_HOLE();

	te->action = action;
	if (t > secs(1)) {
		te->seconds = t/secs(1);
		te->dueDate = get_dueDate(t%secs(1));
	} else {
		te->seconds = 0;
		te->dueDate = get_dueDate(t);
	}
	schedule_te(te);)
}

static void do_action(TimeEvent * te) {
		vector action = te->action;
		te_return(te);
		action();
}

void check_dueDates(Short dueDate) {
	last_dueDate = dueDate;
	
	while (te_todo.next) {
		if (te_todo.next->dueDate != last_dueDate) {
			set_dueDate(te_todo.next->dueDate);
			return;
		}
		TimeEvent* te = te_todo.next;
		te_todo.next = te->next;
		if (te->seconds) {
			te->seconds--;
			te->dueDate = get_dueDate(ONE_SECOND);
			schedule_te(te);
		} else
			do_action(te);
	}
}

static void init_time() {
	te_todo.next = te_done.next = NULL;

	for (Byte i = 0; i < NUM_TE; i++) {
		te_return(&tes[i]);
	}
	zero_ms = last_dueDate = get_dueDate(0);
	later(one_second);
}

// Events
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

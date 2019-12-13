#include "clocks.h"
#include "tea.h"
#include "queue.h"
#include "printers.h"

// 16 bit time tracker; ms and S
static Long uptime = 0;
static Long last_dueDate, zero_ms; // points on the number wheel

static void one_second() {
	uptime++;
	zero_ms = last_dueDate;
	in(secs(1), one_second);
}

Long getTime() { 
	Long ms = to_msec(get_dueDate(0)) - zero_ms;
	return uptime*1000 + ms;
}

uint32_t getUptime() {
	return uptime;
}

void printUptime() {
    printDec(getUptime());
}

// Time Events
static TimeEvent tes[NUM_TE], te_done, te_todo; // time events and todo and done lists

static void append(TimeEvent * curr, TimeEvent * te) {
	te->next = curr->next;
	curr->next = te;
}

static TimeEvent * remove(TimeEvent * curr) {
	TimeEvent * te = curr->next;
	curr->next = te->next;
	te->next = NULL;
	return te;
}

static void te_return(TimeEvent* te) { append(&te_done, te); }

static TimeEvent* te_borrow() {
	if (te_done.next == NULL)
		BLACK_HOLE();

	return remove(&te_done);	
}

static void do_action(TimeEvent * te) {
	bool asap = te->asap;
	vector action = te->action;
	te_return(te);

	if (asap)
		now(action);
	else
		later(action);
}

static void run_dueDate() { do_action(remove(&te_todo)); }

static void set_next_dueDate() {
	while (te_todo.next && !set_dueDate(te_todo.next->dueDate))
		run_dueDate();
}

static void schedule_te(TimeEvent* te) {
	TimeEvent * curr = &te_todo, * next;
	Long ref = last_dueDate; // time reference
	int dueDate = te->dueDate - ref;

	while ((next = curr->next) != NULL) {
		int date = next->dueDate - ref;
		if (dueDate < date)
			break;
		curr = next;
	}
	append(curr, te);
	set_next_dueDate();
}

static void in_after(Long t, vector action, bool asap) {
	CORE_ATOMIC_SECTION(
	TimeEvent * te = te_borrow();

	te->action = action;
	te->dueDate = get_dueDate(t);
	te->asap = asap;
	schedule_te(te);)
}

void after(Long t, vector action) { in_after(t, action, false); }
void in   (Long t, vector action) { in_after(t, action, true);  }

void check_dueDates(Long dueDate) {
	last_dueDate = dueDate;
	set_next_dueDate();
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
void no_action() {}

void when(Event e, vector a) { *e = a; }
void never(Event e) { when(e, no_action); }

// Actions
static QUEUE(NUM_ACTIONS, actionq);

void later(vector a) {
	if (leftq(actionq) == 0)
		BLACK_HOLE();
	CORE_ATOMIC_SECTION( pushq((Cell)a, actionq);)
}

void run() {
	while (queryq(actionq)) {
		vector action = (vector)pullq(actionq);
		action();
	}
}

void runMachines() { // like run but only once through the queued actions; full slice
	Long n = queryq(actionq);
	while (n--) ( (vector)pullq(actionq) )();
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

void print_te() {
	TimeEvent * curr = te_todo.next;
	print("\ncounter:"), printDec(get_counter());
	print("  compare:"), printDec(get_compare());
	while (curr) {
		print("\nDD:"), printDec(curr->dueDate);
		print("  asap:"), printDec(curr->asap);
		print("  action:"), printHex((Cell)curr->action - 1);
		curr = curr->next;
	}
}

void print_actions() {
	int n = queryq(actionq);
	print("\n#actions:"), printDec(n);
}

#include "clocks.h"
#include "tea.h"
#include "queue.h"
#include "printers.h"
#include <stdlib.h>

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

Long end1, end2, end3;
#include "codeStats.c"
#define max(a,b) a > b ? a : b

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
}

void print_ste() {
	print("E1:"), print_elapsed_time(end1);
	print("E2:"), print_elapsed_time(end2-end1);
	print("E3:"), print_elapsed_time(end3-end2);
}

void print_elapsed_time(Cell time) {
	Long us = CONVERT_TO_US(time);
	if (us > 9999)
		dotnb(7, 6, CONVERT_TO_MS(time), 10), print(" ms  ");
	else if (us)
		dotnb(7, 6, us, 10), print(" us  ");
	else
		dotnb(7, 6, CONVERT_TO_NS(time), 10), print(" ns  ");
}

static void in_after(Long t, vector action, bool asap) {
	Long time = getTicks();
	TimeEvent * te;

	safe( te = te_borrow(); )
	end1 = max(end1, getTicks() - time);

	te->action = action;
	te->dueDate = get_dueDate(t);
	te->asap = asap;

	safe(schedule_te(te);)
	end2 = max(end2, getTicks() - time);
	set_next_dueDate();
	end3 = max(end3, getTicks() - time);
}

void after(Long t, vector action) { in_after(t, action, false); }
void in   (Long t, vector action) { in_after(t, action, false);  }
// void in   (Long t, vector action) { in_after(t, action, true);  }

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
	while (queryq(actionq))  actionRun((vector)pullq(actionq));
}

void runMachines() { // like run but only once through the queued actions; full slice
	Long n = queryq(actionq);
	while (n--) actionRun((vector)pullq(actionq));
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

// names for actions
HASHDICT(HASH9, teanames); // keep track of machine names
HASHDICT(HASH9, teatimes); // keep track of machine max execution times

void actor(vector action, const char * name) { // give name to machine
	dictAddKey((Cell)action, &teanames);
	dictAddKey((Cell)action, &teatimes);
	*dictAdjunctKey((Cell)action, &teanames) = (Cell)name;
//	printHex(action), print(name), printCr();
}

void printActionName(Cell x) {
	char ** name = (char **)dictAdjunctKey(x, &teanames);
	if (name)
		print(*name);
	else
		printHex(x-1);
}

void printDueDate(Long dd) {
	dd = dd - get_dueDate(0);
	if (dd < secs(1))
		printDec(to_msec(dd)), print("msec  ");
	else if (dd < mins(5))
		printDec(to_secs(dd)), print("secs  ");
	else if (dd < hours(5))
		printDec(dd/mins(1)),  print("mins  ");
	else
		printDec(dd/hours(1)), print("hours ");
}

void print_te() {
	TimeEvent * curr = te_todo.next;
	print("\ncounter:"), printDec(get_counter());
	print("  compare:"), printDec(get_compare());
	while (curr) {
		print (curr->asap ? "\nin " : "\nafter ");
		printDueDate(curr->dueDate);
		tabTo(17);
		printActionName((Cell)curr->action);
		curr = curr->next;
	}
}

void print_actions() {
	int n = queryq(actionq);
	print("\n#actions:"), printDec(n);
	while(n) {
		printCr();
		printActionName(q(actionq));
		rotateq(actionq, 1);
		n--;
	}
}

void dumpTeaNames() {
	for(int i = 0; i < HASH9; i++)
		if (teanamesadjunct[i])
			printCr(), printHex(teanamesadjunct[i]), print((char *)teanamesadjunct[i]);
}

static int indexCompare(const void *a,const void *b) {
	Short *x = (Short *) a;
	Short *y = (Short *) b;
	return teatimes.adjunct[*y] - teatimes.adjunct[*x];
}

void machineStats(void) {
	Short indexes[teatimes.capacity];
	Short j=0;

	for (Short i=0; i<teatimes.capacity; i++)
		if (teatimes.adjunct[i] != 0)
			indexes[j++] = i;

	qsort(indexes, j, sizeof(Short), indexCompare);

	print(" mstats ");
	for (Short i=0; i<j; i++) {
		Cell machine = (Cell)teatimes.table[indexes[i]];
		char * name = (char *)teanames.adjunct[indexes[i]];
		printCr();
		print_elapsed_time(teatimes.adjunct[indexes[i]]);
		if (name)
			print(name);
		else
			printHex(machine - 1);
	}
}

void zeroMachineTimes() {
    for (Short i=0; i<teatimes.capacity; i++)
        teatimes.adjunct[i] = 0;
}

#define UNKNOWN "unknown_machines"

void initMachineStats() {
    emptyDict(&teanames);
    emptyDict(&teatimes);
//    actor((vector)(Cell)UNKNOWN, UNKNOWN);
	zeroMachineTimes();
}

void actionRun(vector m) {
	Cell * stat = dictAdjunctKey((Cell)m, &teatimes);
	if (stat == 0) {
		// printHex(m), printCr();
		actor(m, NULL);
		stat = dictAdjunctKey((Cell)m, &teatimes);
	}

	int time = (int)getTicks();
	m();
	time = (int)getTicks() - time;

	if (time > (int)*stat) // TODO: consider atomicity of readnwrite
		*stat = (Cell)time;
}

// init
void init_tea() {
	initMachineStats();
	zeroq(actionq);
	init_time();
	init_clocks();
	namedAction(one_second);
	namedAction(no_action);
}

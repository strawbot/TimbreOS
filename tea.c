// Time Event Action interface

#include "tea.h"
#include "queue.h"
#include <stdlib.h>
#include "clocks.h"
#include "project_defs.h"
#ifndef	MINIMAL_TEA
#include "printers.h"
#endif

extern Event alarmEvent;

// time tracker; ms and S
static Long uptime = 0;
static Long last_dueDate; // points on the number wheel

static __reentrant void one_second(void) {
    printf("One second\r\n");
	in(secs(1), one_second);
	uptime++;
}

Long getTime(void) { 
	return (to_msec(raw_time()));
}

#ifndef	MINIMAL_TEA
Long get_uptime() {
	return uptime;
}

void printUptime() {
    printDec(get_uptime());
}
#endif

// primitives
static TimeEvent tes[NUM_TE], te_done, te_todo; // time events and todo and done lists

static void verify_todo(void) {
	TimeEvent * te = &te_todo;
	while ((te = te->next)) ;
}

static void append(TimeEvent * curr, TimeEvent * te) {
	safe(
		te->next = curr->next;
		curr->next = te;
	)
}

static TimeEvent * remove_te(TimeEvent * curr) {
	TimeEvent * te;
	safe(
		te = curr->next;
		curr->next = te->next;
	)
	te->next = NULL;
	return te;
}

// list access
static void te_return(TimeEvent* te) { append(&te_done, te); }

static TimeEvent* te_borrow(void) {
	if (te_done.next == NULL)
		BLACK_HOLE();

	return remove_te(&te_done);	
}

// make it happen
static void do_action(TimeEvent * te) {
	bool asap = te->asap;
	vector action = te->action;
	te_return(te);

	if (asap)
		now(action);
	else
		later(action);
}

static void run_dueDate(void) {
	do_action(remove_te(&te_todo));
	verify_todo();
}

static Long get_dueDate(Long t) { return raw_time() + t; }

static Long max_delta = 0;

static bool set_dueDate(Long due) {
	 // must be signed since for overdue items
	int delta = (int) (due - raw_time());

	if (delta > 0) {
		set_alarm((Long) delta);
		return true;
	}
	
	if (delta < 0) {
		max_delta = (Long) -delta > max_delta ? (Long) -delta : max_delta;
		over_due();
	}
	return false;
}

static void set_next_dueDate(void) {
	while (te_todo.next && !set_dueDate(te_todo.next->dueDate)) // atomicity issue?
		run_dueDate();
}

static void schedule_te(TimeEvent* te) {
	TimeEvent * curr = &te_todo, * next;
	Long ref = last_dueDate; // time reference
	int dueDate = (int) (te->dueDate - ref);

	while ((next = curr->next) != NULL) {
		int date = (int) (next->dueDate - ref);
		if (dueDate < date)
			break;
		curr = next;
	}
	append(curr, te);
	verify_todo();
}

static TimeEvent * already_there(vector action) {
	TimeEvent *te, *tep = &te_todo;

	while ((te = tep->next)) {
		if (te->action == action) {
			tep->next = te->next;
			return te;
		}
		tep = te;
	}
	return NULL;
}

void in_after(Long t, vector action, bool asap) {
	if (action == no_action)
		return;

	TimeEvent * te;

	safe( te = already_there(action); )
	
	if (!te) {
		safe( te = te_borrow(); )
	}
	
	te->action = action;
	te->dueDate = get_dueDate(t);
	te->asap = asap;

	safe(schedule_te(te);)
	set_next_dueDate();
}

// Time Events
void after(Long t, vector action) { in_after(t, action, false); }
void in   (Long t, vector action) { in_after(t, action, true); }

static void check_dueDates(void) {
	last_dueDate = raw_time();
	set_next_dueDate();
}

// Events
void no_action(void) {}

void when(Event e, vector a) { *e = a; }
void never(Event e) { when(e, no_action); }

// Actions
static QUEUE(NUM_ACTIONS, actionq);

void later(vector a) {
	if (a == NULL)
		BLACK_HOLE();
	if (leftq(actionq) == 0)
		BLACK_HOLE();
	safe( pushq((Cell)a, actionq);)
}

void run(void) {
	while (queryq(actionq))  actionRun((vector)pullq(actionq));
}

void action_slice(void) { // like run but only once through the queued actions; full slice
	Long n = queryq(actionq);
	while (n-- && queryq(actionq))
		actionRun((vector)pullq(actionq));
}

// reductions
void stop_te(vector v) {
	safe(
	TimeEvent* te = te_todo.next;
	while (te) {
		if (te->action == v)  te->action = no_action;
		te = te->next;
	})
}

void stop_action(vector v) {
	safe(
	for (Long n = queryq(actionq); n; n--) {
		Cell a = pullq(actionq);
		if (a != (Cell)v)  pushq(a, actionq); // develop iter q tool to read and write without moving items
	})
}

void stop(vector v) {
	stop_te(v);
	stop_action(v);
}

// Tools

#ifndef	MINIMAL_TEA
// times
void print_time(Long time) {
	if (time < secs(1))
		printDec(to_msec(time)), print("msec  ");
	else if (time < mins(5))
		printDec(to_secs(time)), print("secs  ");
	else if (time < hours(5))
		printDec(time/mins(1)),  print("mins  ");
	else
		printDec(time/hours(1)), print("hours ");
}

static void printDueDate(Long dd) {
	print_time(dd - get_dueDate(0));
}

// named actions 
HASHDICT(HASH9, teanames); // keep track of machine names
HASHDICT(HASH9, teatimes); // keep track of machine max execution times

void actor(vector action, const char * name) { // give name to action
	dictAddKey((Cell)action, &teatimes);
	dictAddKey((Cell)action, &teanames);
	*dictAdjunctKey((Cell)action, &teanames) = (Cell)name;
}

void printActionName(Cell x) {
	char ** name = (char **)dictAdjunctKey(x, &teanames);
	if (name && name[0] != 0)
		print(*name);
	else
		printHex(x-1);
}

void print_te() {
	TimeEvent * curr = te_todo.next;
	show_timer();
	while (curr &&
		  (curr->action != NULL) && 
		  (curr->dueDate - get_dueDate(0) < hours(36))) {
		print (curr->asap ? "\nin " : "\nafter ");
		printDueDate(curr->dueDate);
		tabTo(17);
		printActionName((Cell)curr->action);
		curr = curr->next;
	}
	if (curr)
		print("\n  Stopped. TE list changed while printing it.");
	print("\nmax overdue: "), printDec(max_delta);
	max_delta = 0;
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

// ns 32 bit clock @ native MHz. Clock.h
#include "codeStats.c"

static int indexCompare(const void *a,const void *b) {
	Short *x = (Short *) a;
	Short *y = (Short *) b;
	return teatimes.adjunct[*y] - teatimes.adjunct[*x];
}

static void print_elapsed_time(Cell time) {
	Long us = CONVERT_TO_US(time);
	if (us > 9999)
		dotnb(7, 6, CONVERT_TO_MS(time), 10), print(" ms  ");
	else if (us)
		dotnb(7, 6, us, 10), print(" us  ");
	else
		dotnb(7, 6, CONVERT_TO_NS(time), 10), print(" ns  ");
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
			printHex((machine + 3) & ~3);
	}
}

Cell * action_stat(vector m) {
	Cell * stat = dictAdjunctKey((Cell)m, &teatimes);
	if (stat == 0) {
		actor(m, NULL);
		stat = dictAdjunctKey((Cell)m, &teatimes);
	}
	return stat;
}
#endif

void actionRun(vector m) {
	if (m == 0)
		BLACK_HOLE();
#ifndef	MINIMAL_TEA
	Cell * stat = action_stat(m);
	int time = (int)getTicks();
#endif
	m();
#ifndef	MINIMAL_TEA
	Long delta = (int)getTicks() - time;

	if (delta > *stat)
		*stat = delta;
#endif
}

#ifndef	MINIMAL_TEA
void zeroMachineTimes() {
    for (Short i=0; i<teatimes.capacity; i++)
        teatimes.adjunct[i] = 0;
}

void initMachineStats() {
    emptyDict(&teanames);
    emptyDict(&teatimes);
	zeroMachineTimes();
}
#endif

// test vector
#include "timeout.h"
#include "cli.h"

void ticks_ms(void) { lit((Cell) CONVERT_TO_MS(ret())); }

#ifndef	MINIMAL_TEA
void test_time() {
	Long s = ret();
	Long tick = getTicks();
	Long time = getTime();
	Long raw = raw_time();
	timeoutWait(secs(s));
	tick = getTicks() - tick;
	time = getTime() - time;
	raw = raw_time() - raw;
	print("\n"), printDec0(s), print(" second in ticks to ms: "), printDec(CONVERT_TO_MS(tick));
	print("  in time(ms):"), printDec(time);
	print("  in raw:"), printDec(raw);
}

void get_tick_time() {
	Long tick = getTicks();
	Long time = getTime();
	printDec(tick), printDec(time);
}

// can use for tracing events; override defaults as needed
QUEUE(N_EVENTS * 2, eventq);
static bool playback = false;

static void record(const char * e) {
	if (queryq(eventq) == 0) {
		if (e != FIRST_EVENT)
			return;

		after((Cell)FIRST_EVENT, play_events);
	}
	if (leftq(eventq) > 2) {
		pushq((Cell)e, eventq);
		pushq(getTime(), eventq);
	} else
		playback = true;
}

void record_event(const char * e) {
	if (playback == false)
		safe(record(e);)
}

void play_events() {
	Long zero = 0, last = ~0;
	playback = true;
	while (queryq(eventq)) {
		char * e = (char *)pullq(eventq);
		Long t = pullq(eventq);

		if (e == FIRST_EVENT) {
			if (zero == 0)
				zero = t;
			e = "FIRST_EVENT";
		}

		if (t != last) {
			printCr();
			printDec(t-zero);
			tabTo(6);
		} else{ 
			last = t;
			print("  ");
		}
		print(e);
	}
	playback = false;
}
#endif

// init
void init_tea() {
#ifndef	MINIMAL_TEA
	initMachineStats();
#endif
	zeroq(actionq);
	te_todo.next = te_done.next = NULL;

	for (Byte i = 0; i < NUM_TE; i++)
		te_return(&tes[i]);

	last_dueDate = get_dueDate(0);

	later(one_second);
	init_clocks();
	when(alarmEvent, check_dueDates);

#ifndef	MINIMAL_TEA
	namedAction(set_next_dueDate);
	namedAction(one_second);
	namedAction(no_action);
	namedAction(check_dueDates);
	namedAction(play_events);
#endif
}

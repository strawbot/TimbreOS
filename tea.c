// Time Event Action interface

#include <Board/clocks.h>
#include "tea.h"
#include "queue.h"
#include "printers.h"
#include <stdlib.h>
#include <string.h>
#include "project_defs.h"

#define MIN_DELTA 2 // minimum worth queueing up, ~200us

// time tracker; ms and S
static volatile Long last_dueDate; // points on the number wheel

Long getTime() { 
	return (to_msec(get_ticks()));
}

// primitives
Event alarmEvent = { no_action };
static TimeEvent te_done, te_todo, tes[NUM_TE]; // time events with todo and done lists
static Byte te_min_left, te_left;

static void prepend(TimeEvent * curr, TimeEvent * te) {
	te->next = curr->next;
	curr->next = te;
}

static TimeEvent * remove(TimeEvent * curr) {
	TimeEvent * te;
	te = curr->next;
	curr->next = te->next;
	te->next = NULL;
	return te;
}

void te_lists() {
	for (Byte ix = 0; ix < NUM_TE; ix++) {
		printCr(), printDec(ix), printChar(' ');
		if (tes[ix].next)
			printDec0(tes[ix].next - tes);
		else
			print("end");
	}
	print("\ntodo, done: "), printDec(te_todo.next - tes), printDec(te_done.next - tes);
}

static void verify_todo() {
	TimeEvent * te = &te_todo;
	Byte ix = NUM_TE;
	while(ix--)
		if ((te = te->next) == 0) {
			te = te_done.next;
			while (ix--)
				if ((te = te->next) == 0) {
					never(alarmEvent);
					print("\nTE list missing tes: "), printDec(ix);
					te_lists();
					flush();
					BLACK_HOLE(TE_MISSING);
				}
			return;
		}
	print("\nTE Looped:");
	for (ix = 0; ix < NUM_TE; ix++)
		printCr(), printDec(ix), printDec((tes[ix].next - tes)/sizeof(TimeEvent));
	flush();
	BLACK_HOLE(TE_LOOPED);
}

Byte te_min() { return te_min_left; }

// list access
static void te_return(TimeEvent* te) {
	prepend(&te_done, te);
	te_left++;
}

static TimeEvent* te_borrow() {
	if (te_done.next == NULL)
		BLACK_HOLE(TE_UNDERRUN);

	te_left--;
	if (te_left < te_min_left)
		te_min_left = te_left;

	return remove(&te_done);
}

// make it happen
static void do_action(TimeEvent * te) {
	bool asap = te->asap;
	vector action = te->action;
	te_return(te);

	if (asap)
		action();
	else
		later(action);
}

static void run_dueDate() {
	do_action(remove(&te_todo));
	verify_todo();
}

static Long get_dueDate(Long t) { return get_ticks() + t; }

static Long max_delta = 0;

static bool set_dueDate(Long due) {
	 // must be signed since for overdue items
	int delta = due - get_ticks();

	if (delta > MIN_DELTA) {
		set_delta_alarm(delta);
		return true;
	}
	
	if (delta < 0) {
		max_delta = (Long) -delta > max_delta ? (Long) -delta : max_delta;
		over_due();
	}
	return false;
}

static void set_next_dueDate() {
	while (te_todo.next && !set_dueDate(te_todo.next->dueDate)) // atomicity issue?
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
	prepend(curr, te);
	verify_todo();
	if (te_todo.next == te)
	    set_next_dueDate();
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

static void in_after(Long t, vector action, bool asap) {
	if (action && action != no_action) {
		if ((int)t > 0) { // deal with unsigned negatives
			safe(
				TimeEvent * te = already_there(action);

				if (!te)
					te = te_borrow();
				
				te->action = action;
				te->dueDate = get_dueDate(t);
				te->asap = asap;

				schedule_te(te);
			)
		} else {
			if (asap)
				action();
			else
				later(action);
		}
	}
}

// Time Events
void after(Long t, vector action) { in_after(t, action, false); }
void in   (Long t, vector action) {
	if (t == 0)
		BLACK_HOLE(IN_0);
	in_after(t, action, true);
}

static void check_dueDates() { // delta interrupt handler
	last_dueDate = get_ticks();
	set_next_dueDate();
}

// Events
void no_action() {}

void when(Event e, vector a) { *e = a; }
void never(Event e) { when(e, no_action); }

// Actions
static QUEUE(NUM_ACTIONS, actionq);

void later(vector a) {
	if (a == NULL)
		BLACK_HOLE(LATER_NULL);
	if (leftq(actionq) == 0)
		BLACK_HOLE(ACTION_OVERRUN);
	if (a != no_action)
		safe( pushq((Cell)a, actionq);)
}

void run() {
	while (queryq(actionq))  actionRun((vector)pullq(actionq));
}

void action_slice() { // like run but only once through the queued actions; full slice
	Long n = queryq(actionq);
	while (n-- && queryq(actionq))
		actionRun((vector)pullq(actionq));
}

// reductions
void stop_te(vector v) {
	safe(
		TimeEvent * te = already_there(v);
		if (te)
			te_return(te);
	)
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

// measure latency for in, after and later to indicate system throughput
//   sequence: start ->1s in ->1s after -> later
//  overhead: 247us every 10,000,000us
#define AVG_WEIGHT 5
#define DATA_WEIGHT 2
enum {DATA,MIN,MAX,AVG,TABLE};
enum {IN,AFTER,LATER,MEASURES};
static int metrix[TABLE][MEASURES];
static Long stamps[MEASURES];
static void measure_latency();

static void init_latency() {
	metrix[MIN][IN] = metrix[MIN][AFTER] = metrix[MIN][LATER] = to_usec(secs(1));
	metrix[MAX][IN] = metrix[MAX][AFTER] = metrix[MAX][LATER] = to_usec(-secs(1));
	metrix[AVG][IN] = metrix[AVG][AFTER] = metrix[AVG][LATER] = 0;
}
/*
latency in ticks:  min   max   avg
in(secs(1),)       2     2     0
after(secs(1),)    2     2     0
later()            0     0     0
*/
void show_latency() {
	print("\nlatency in us:     min     max     avg");
	char * measure[3] = {"\nin(secs(1),)", "\nafter(secs(1),)", "\nlater()"};
	for (Byte m=IN; m<MEASURES; m++) {
		print(measure[m]);
		for (Byte i=MIN; i<TABLE; i++) //void dotnb(Byte field, Byte digits, Cell n, Byte radix);
			tabTo(17+(i-MIN)* 8), dotnb(5,0,metrix[i][m],10);
	}
}

void reset_latency() {
	for (Byte i = IN; i < MEASURES; i++) {
		metrix[MIN][i] = to_usec(secs(1));
		metrix[MAX][i] = to_usec(-secs(1));
		metrix[AVG][i] = 0;
	}
}

static void measure_later() {//start,in,after,later
	// calculate
	metrix[DATA][LATER] = to_usec(get_ticks()) - to_usec(stamps[LATER]);
	metrix[DATA][AFTER] = to_usec(stamps[LATER] - stamps[AFTER]) - to_usec(secs(1));
	metrix[DATA][IN] = to_usec(stamps[AFTER] - stamps[IN]) - to_usec(secs(1));
	// tabulate
	for (Byte i = IN; i < MEASURES; i++) {
		metrix[MIN][i] = min(metrix[DATA][i], metrix[MIN][i]);
		metrix[MAX][i] = max(metrix[DATA][i], metrix[MAX][i]);
		metrix[AVG][i] = (DATA_WEIGHT * metrix[DATA][i] + 
						  AVG_WEIGHT * metrix[AVG][i]) /
						 (DATA_WEIGHT + AVG_WEIGHT);
	}
	after(secs(10), measure_latency);
}
static void measure_after() { stamps[LATER] = get_ticks(); later(measure_later); }
static void measure_in() { stamps[AFTER] = get_ticks(); after(secs(1),measure_after); }
static void measure_latency() { stamps[IN] = get_ticks(); in(secs(1), measure_in); }

// named actions
HASHDICT(TEA_TABLE, teanames); // keep track of machine names
HASHDICT(TEA_TABLE, teatimes); // keep track of machine max execution times

static char cname[32] = {0};

static void bad_name() { print("!"),print(cname),print("  check c name "); cname[0] = 0; }

void actor(vector action, const char * name) { // give name to action
	Cell key = 0xFFFFFFFC & ((Cell)action + 1);
	if (dictFindKey(key, &teanames) == 0) {
		dictAddKey(key, &teatimes);
		dictAddKey(key, &teanames);
		*dictAdjunctKey(key, &teanames) = (Cell)name;
	} else if (cname[0] != 0 ) {
		strncpy(cname, name, 31);
		later(bad_name);
	}
}

void printActionName(Cell key) {
	key = 0xFFFFFFFC & (key+1);
	char ** name = (char **)dictAdjunctKey(key, &teanames);
	if (name && name[0] != 0)
		print(*name);
	else
		printHex(key);
}

void print_te() {
	TimeEvent * curr = te_todo.next;
	show_timer();
	while (curr &&
		  (curr->action != NULL) //&& 
		//   (curr->dueDate - get_dueDate(0) < hours(36))
		  ) {
		print (curr->asap ? "\nin " : "\nafter ");
		printDueDate(curr->dueDate);
		tabTo(17);
		printActionName((Cell)curr->action);
		curr = curr->next;
	}
	if (curr)
		print("\n  Stopped. TE list changed while printing it.");
	print("\nmax overdue: "), printDec(max_delta);
	print("  te min: "), printDec(te_min());
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
#include "codeStats.h"

static int indexCompare(const void *a,const void *b) {
	Short *x = (Short *) a;
	Short *y = (Short *) b;
	return teatimes.adjunct[*y] - teatimes.adjunct[*x];
}

static void print_elapsed_time(Cell time) {
	Long us = SYS_TO_US(time);
	if (us > 9999)
		dotnb(7, 6, SYS_TO_MS(time), 10), print(" ms  ");
	else if (us)
		dotnb(7, 6, us, 10), print(" us  ");
	else
		dotnb(7, 6, SYS_TO_NS(time), 10), print(" ns  ");
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
			printHex((machine + 2) & ~3);
	}
	show_latency();
}

Cell * action_stat(vector m) {
	Cell * stat = dictAdjunctKey((Cell)m, &teatimes);
	if (stat == 0) {
		actor(m, NULL);
		stat = dictAdjunctKey((Cell)m, &teatimes);
	}
	return stat;
}

void actionRun(vector m) {
	if (m == 0)
		BLACK_HOLE(ACTION_NULL);
	if (m == no_action)
		return;
	Cell * stat = action_stat(m);
	int time = (int)sysTicks();
	m();
	Long delta = (int)sysTicks() - time;

	if (delta > *stat)
		*stat = delta;
}

void show_key_costs() { dump_key_hashes(&teatimes); }

void zeroMachineTimes() {
	for (Short i=0; i<teatimes.capacity; i++)
    	teatimes.adjunct[i] = 0;
	init_latency();
}

void initMachineStats() {
    emptyDict(&teanames);
    emptyDict(&teatimes);
	zeroMachineTimes();
}

// test vector
#include "timeout.h"
#include "cli.h"

void test_time() {
	Long s = ret();
	Long tick = sysTicks();
	Long time = getTime();
	Long raw = get_ticks();
	timeoutWait(secs(s));
	tick = sysTicks() - tick;
	time = getTime() - time;
	raw = get_ticks() - raw;
	print("\n"), printDec0(s), print(" second in ticks to ms: "), printDec(SYS_TO_MS(tick));
	print("  in time(ms):"), printDec(time);
	print("  in raw:"), printDec(raw);
}

void get_tick_time() {
	Long tick = sysTicks();
	Long time = getTime();
	printDec(tick), printDec(time);
}

void ticks_ms() { lit(SYS_TO_MS(ret())); }

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

void record_interrupt(const char * e) {
	if (IN_INTERRUPT())
		record_event(e);
}

void record_event_off() { playback = true; }

void record_events_on() { playback = false;  zeroq(eventq);  record(FIRST_EVENT); }

void play_events() {
	bool fe = false;
	Long zero = 0, last = ~0;
	playback = true;
	while (queryq(eventq)) {
		char * e = (char *)pullq(eventq);
		Long t = pullq(eventq);

		if (e == FIRST_EVENT) {
			if (zero == 0) {
				zero = t;
				fe = true;
			}
			e = "FIRST_EVENT";
		}

		if (t != last) {
			flush(), maybeCr();
			if (fe)
				fe = false;
			else
				print(" +"), printDec(t - zero);
			printChar(' ');
			tabTo(8);
		} else { 
			last = t;
			print("  ");
		}
		print(e);
	}
	playback = false;
}

// init
void init_tea() {
	initMachineStats();
	zeroq(actionq);
	te_todo.next = te_done.next = NULL;
	te_left = 0;
	for (Byte i = 0; i < NUM_TE; i++)
		te_return(&tes[i]);
	te_min_left = NUM_TE;

	last_dueDate = get_dueDate(0);

	namedAction(measure_latency);
	namedAction(measure_in);
	namedAction(measure_after);
	namedAction(measure_later);
	namedAction(set_next_dueDate);
	namedAction(no_action);
	namedAction(check_dueDates);
	// namedAction(play_events);

	init_clocks();
	when(alarmEvent, check_dueDates);
	later(measure_latency);
}

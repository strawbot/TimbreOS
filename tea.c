// Time Event Action interface

#include "tea.h"
#include "queue.h"
#include "printers.h"
#include "ttypes.h"
#include <stdlib.h>
#include <string.h>

#define MIN_DELTA 2 // minimum worth queueing up, ~200us

// time tracker; updated each time set_delta_alarm is called
static volatile Long last_load_ticks;

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

static TimeEvent * unlink_te(TimeEvent * curr) {
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
	print("\ntodo: ");
	if (te_todo.next) printDec(te_todo.next - tes); else print("null");
	print("  done: ");
	if (te_done.next) printDec(te_done.next - tes); else print("null");
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

	return unlink_te(&te_done);
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
	do_action(unlink_te(&te_todo));
	// verify_todo();
}

static Long max_delta = 0;

// delta_alarm_handler advances last_load_ticks by each head's delta before firing
// it, so that schedule_te called from inside an action gets a fresh reference.
static void delta_alarm_handler() {
    static bool running = false;
    // print("H"); // PROBE: handler entered
    if (running) return;
    running = true;
    while (te_todo.next) {
        Long elapsed = get_ticks() - last_load_ticks;
        Long d = (Long)te_todo.next->delta;
        if (elapsed + MIN_DELTA < d) {
            Long load = d - elapsed;
            te_todo.next->delta = load;
            set_delta_alarm(load);
            last_load_ticks = get_ticks();
            break;
        }
        Long overshoot = elapsed - d;
        if (overshoot > max_delta) max_delta = overshoot;
        if (overshoot > 0) over_due();
        last_load_ticks += d; // advance reference to head's intended fire time
        run_dueDate();
    }
    running = false;
}

static void schedule_te(TimeEvent *te) {
    Long elapsed = get_ticks() - last_load_ticks;
    Long te_abs = (Long)te->delta + elapsed; // fire time relative to last_load_ticks

    // Remove duplicate (same action) from list; add its delta to its successor
    // so the successor's absolute fire time is unchanged.
    {
        TimeEvent *p = &te_todo;
        while (p->next) {
            if (p->next->action == te->action) {
                TimeEvent *dup = p->next;
                p->next = dup->next;
                if (dup->next)
                    dup->next->delta += dup->delta;
                te_return(dup);
                break;
            }
            p = p->next;
        }
    }

    // Find the insertion point (sorted by absolute fire time).
    Long accum = 0;
    TimeEvent *prev = &te_todo;
    while (prev->next) {
        Long next_abs = accum + (Long)prev->next->delta;
        if (next_abs > te_abs)
            break;
        accum = next_abs;
        prev = prev->next;
    }

    // Insert te, splitting the delta at the insertion point.
    te->delta = te_abs - accum;
    te->next = prev->next;
    if (te->next)
        te->next->delta -= te->delta;
    prev->next = te;

    // If te is the new head, program the hardware timer and anchor last_load_ticks
    // so the stored delta and the reference stay consistent.
    if (prev == &te_todo) {
        Long elapsed2 = get_ticks() - last_load_ticks;
        Long load = (Long)te->delta - elapsed2;
        if (load < MIN_DELTA) load = MIN_DELTA;
        te->delta = load;
        set_delta_alarm(load);
        last_load_ticks = get_ticks();
    }
}

static void in_after(Long t, vector action, bool asap) {
	if (action && action != no_action) {
		if ((int)t > 0) { // deal with unsigned negatives
			TimeEvent * te = te_borrow();
			te->action = action;
			te->delta = t; // raw delay; schedule_te converts to list coordinates
			te->asap = asap;
			safe( schedule_te(te); )
		} else {
			if (asap)
				actionRun(action);
			else
				later(action);
		}
	}
}

// Time Events - 0 ok for after but not in since it is a timing error
void after(Long t, vector action) { in_after(t, action, false); }
void in   (Long t, vector action) {
	if (t == 0) // 0 is considered a timing error
		BLACK_HOLE(IN_0);
	in_after(t, action, true);
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
void stop_te(vector action) {
	safe(
		TimeEvent *te;
		TimeEvent *tep = &te_todo;

		while ((te = tep->next)) {
			if (te->action == action) {
				tep->next = te->next;
				if (te->next)
					te->next->delta += te->delta;
				te_return(te);
				break;
			}
			tep = te;
		}
	)
}

void stop_action(vector action) {
	safe(
		for (Long n = queryq(actionq); n; n--) {
			Cell a = pullq(actionq);
			if (a != (Cell)action)  pushq(a, actionq); // develop iter q tool to read and write without moving items
		}
	)
}

void stop(vector action) {
	stop_te(action);
	stop_action(action);
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
static void measure_latency() {
	safe( verify_todo(); )
	stamps[IN] = get_ticks();
	in(secs(1), measure_in);
}

// named actions
HASHDICT(TEA_TABLE, teanames); // keep track of machine names
HASHDICT(TEA_TABLE, teatimes); // keep track of machine max execution times

// only report one duplicate per name to avoid flooding the console;
//  cleared on next duplicate or at print time
static char cname[32] = {0};

static void bad_name() {
	print("!"),print(cname),print("  check c name ");
	cname[0] = 0; // signal ready for use
}

static Cell action_key(vector action) { return ~(Cell)3 & ((Cell)action + 1); }

void actor(vector action, const char * name) { // give name to action
	Cell key = action_key(action);
	if (dictFindKey(key, &teanames) == 0) { // check for duplicate
		dictAddKey(key, &teatimes);
		dictAddKey(key, &teanames);
		*dictAdjunctKey(key, &teanames) = (Cell)name;
	} else if (cname[0] == 0 && name[0] != 0 ) { // only report if there is a name
		strncpy(cname, name, 31);
		later(bad_name);
	}
}

void printActionName(Cell key) {
	key = action_key((vector) key);
	char ** name = (char **)dictAdjunctKey(key, &teanames);
	if (name && name[0] != 0)
		print(*name);
	else
		printHex(key);
}

void print_te() {
    TimeEvent *curr = te_todo.next;
    show_timer();
    Long elapsed = get_ticks() - last_load_ticks;
    // print("\nllt:"), printDec(last_load_ticks);   // PROBE
    // print("  now:"), printDec(get_ticks());        // PROBE
    // print("  el:"), printDec(elapsed);             // PROBE
    Long accum = 0;
    while (curr && curr->action != NULL) {
        accum += (Long)curr->delta;
        // print("\n  raw delta:"), printDec(curr->delta);   // PROBE
		Long time_from_now = accum > elapsed ? accum - elapsed : 0;
		print(curr->asap ? "\nin " : "\nafter ");
		print_time(time_from_now);
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
	Cell *n = teanames.adjunct;
	Cell *t = teatimes.adjunct;
	for(int i = 0; i < teanames.capacity; i++)
		if (n[i])
			printCr(),printHex((Cell)teanames.table[i]),print((char *)n[i]),print("  "),printDec(t[i]);
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
	else if (us > 3)
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

	print(" mstats: "), printDec(j);
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
	Cell key = action_key(m);
	Cell * stat = dictAdjunctKey(key, &teatimes);
	if (stat == 0) {
		actor(m, NULL);
		stat = dictAdjunctKey(key, &teatimes);
	}
	return stat;
}

void actionRun(vector m) {
	if (m == 0)
		BLACK_HOLE(ACTION_NULL);
	if (m == no_action)
		return;
	Cell * stat = action_stat(m);
	Long time = sysTicks();
	m();
	Long delta = sysTicks() - time;
	if (stat && delta > *stat)
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
	printuDec(tick), print(", "),printuDec(time);
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
		} else
			print("  ");
		
		print(e);
	}
	playback = false;
}

// init
void init_tea() {
	te_todo.next = te_done.next = NULL;
	te_left = 0;
	for (Byte i = 0; i < NUM_TE; i++)
		te_return(&tes[i]);
	te_min_left = NUM_TE;

	last_load_ticks = get_ticks();

	namedAction(measure_latency);
	namedAction(measure_in);
	namedAction(measure_after);
	namedAction(measure_later);
	namedAction(no_action);
	namedAction(delta_alarm_handler);
	// namedAction(play_events);

	init_clocks();
	when(alarmEvent, delta_alarm_handler);
	later(measure_latency);
}

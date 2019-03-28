// take an action after time
/*
      TimeEvent some_te;

          after(te_msecs(25), some_action);
          every(te_msecs(25), some_action);

      stoptimeevent(some action);

        The interrupt to run the checkTimeouts for all the after's is
   independant
        of the timeout times. The timeout times are based on BURTC timer. The
        RTC timer is how often the timeouts are checked. If there is one timeout
        and it is for 1 second, then the RTC should be running at least twice
   that
        rate to give it good accuracy or 500mS.  If a timeout is 10ms, then the
        RTC should run at 5mS. At some point, the interrupt will run too often
        and all the interrupt time will be spent checking timeouts hogging all
        the CPU time. So there should be a lower bound on the RTC setting.

        A simple system sets RTC time to the minimum value and then everything
        just uses that. But this is not the most efficient system. This can be
        obtained dynamically changing the RTC interrupt rate. To make this
   easier,
        there should be multiple lists. One for each timeouts that fall into the
        classes: > 10 ms, 100 ms, 1 S. When an after or timeout is queued to be
        listed, it is sorted to the list of its type. When a list is empty, then
        the RTC counter can be set to a different time interval. When running
   all
        three lists, the 10ms list is checked each interrupt. The 100ms list is
        checked each 10 interrupts and the 1S list is checked each 100
   interrupts.

        after will run an action after some time while every is similar the
        difference is that "every" is periodic from the first time the action
        executes whereas "after" will run after the time but there may be some
        other time in there as well.

*/
extern "C" {
#include "machines.h"
#include "printers.h"
#include <string.h>

static void relist(TimeEvent* te);

// time actions
static QUEUE(30, timeeventq);

static TimeEvent timeeventList;

static Integer time_left(Timeout* timer) {
    Integer elapsed = getTime() - timer->timeset;
    Integer interval = (Integer)timer->timeout;

    return interval - elapsed;
}

void printAction(Action * a) {
    char* name;
    if (a->method == jump)
        name = getMachineName((Cell)a->object);
    else
        name = getMachineName((Cell)a->method);
    if (strlen(name))
        print(name);
    else
        printHex((Cell)a->method);
}

void listTimeEvents() {
    TimeEvent* te = &timeeventList;
    print("\nPending timed actions:");
    while ((te = te->link) != NULL) {
        print("\n ");
        printAction(&te->action);
        print(" in "), printDec(time_left(&te->to)), print("ms");
    }
}

static void inlisttimeevents() {
    while (queryq(timeeventq)) {
        TimeEvent* te = (TimeEvent*)pullq(timeeventq);

        te->link = timeeventList.link;
        timeeventList.link = te;
    }
}

static bool time_ready = true;

static void checktimeevents() {
    TimeEvent* before = &timeeventList;
    TimeEvent* te;

    inlisttimeevents();
    while ((te = before->link) != 0) {
        if (checkTimeout(&te->to)) {
            if (!te->action.persist) {
                before->link = te->link;
                relist(te);
            } else {
                before = te;
                repeatTimeout(&te->to);
            }
            if (te->action.method == jump)
                next((vector)te->action.object);
            else
                next(te->action.object, (unafun)te->action.method);
        } else
            before = te;
    }
    time_ready = true;
}

void timeevent_IRQ() {
    if (time_ready) {
        time_ready = false;
        next(checktimeevents);
    }
}

__attribute__((weak)) void timeeventError(TimeEvent* te) {
    printHex((Cell)te), print(" is already in timeevent queue!");
}

void timeevent(TimeEvent* te) {
    if (te->link == LINK_SENTINEL && 0 == scanq((Cell)te, timeeventq)) {
        te->link = nullptr;
        pushq((Cell)te, timeeventq);
    } else {
        timeeventError(te);
        while (true);
    }
}

#define TOTAL_TE 40
static TimeEvent timeevents[TOTAL_TE];

void init_te() {
    timeeventList.link = nullptr;
    for (int i = 0; i < TOTAL_TE; i++) {
        timeevents[i].link = &timeevents[i + 1];
    }
    timeevents[TOTAL_TE - 1].link = 0;
    zeroq(timeeventq);
}

TimeEvent* getTe() {
    TimeEvent* te = timeevents[0].link;

    if (te) {
        timeevents[0].link = te->link;
        te->link = LINK_SENTINEL;
        return te;
    }
    while (true);
    print("\nERROR in getTe: No more timeevents left.");
    return timeevents;
}

static void relist(TimeEvent* te) {
    te->link = timeevents[0].link;
    timeevents[0].link = te;
}

void stopTe(TimeEvent* te) {
    if (te == nullptr) return;

    TimeEvent* before = &timeeventList;
    TimeEvent* tai;

    while ((tai = before->link) != 0) {
        if (tai == te) {
            before->link = te->link;
            relist(te);
            return;
        }
        before = tai;
    }

    for (int n = queryq(timeeventq); n; n--) {
        Cell tap = pullq(timeeventq);
        if (tap == (Cell)te)
            relist(te);
        else
            pushq(tap, timeeventq);
    }
}

void stopTimeEvent(vector action) {
    TimeEvent* before = &timeeventList;
    TimeEvent* tai;

    while ((tai = before->link) != 0) {
        if (tai->action.object == action) {
            before->link = tai->link;
            relist(tai);
        } else
            before = tai;
    }

    for (int n = queryq(timeeventq); n; n--) {
        TimeEvent* tap = (TimeEvent*)pullq(timeeventq);
        if (tap->action.object == action) {
            relist(tap);
        } else
            pushq((Cell)tap, timeeventq);
    }
}

TimeEvent* after(Long time, vector action) {
    TimeEvent* te = getTe();

    setTimeout(time, &te->to);
    te->action.method = (void*)jump;
    te->action.object = (void*)action;
    te->action.persist = false;
    timeevent(te);
    return te;
}

TimeEvent* every(Long time, vector action) {
    TimeEvent* te = after(time, action);
    te->action.persist = true;
    return te;
}

}
// C++
TimeEvent* after(Long time, void* cpp_obj, unafun action, const char * name) {
    TimeEvent* te = getTe();

    setTimeout(time, &te->to);
    te->action.name = name;
    te->action.method = (void*)action;
    te->action.object = cpp_obj;
    te->action.persist = false;
    timeevent(te);
    return te;
}

TimeEvent* every(Long time, void* cpp_obj, unafun action, const char * name) {
    TimeEvent* te = after(time, cpp_obj, action, name);
    te->action.persist = true;
    return te;
}

void stopTimeEvent(void* cpp_obj, unafun action) {
    TimeEvent* before = &timeeventList;
    TimeEvent* tai;

    while ((tai = before->link) != 0) {
        if (tai->action.object == cpp_obj && tai->action.method == action) {
            before->link = tai->link;
            relist(tai);
        } else
            before = tai;
    }

    for (int n = queryq(timeeventq); n; n--) {
        TimeEvent* tap = (TimeEvent*)pullq(timeeventq);
        if (tap->action.object == cpp_obj && tap->action.method == action) {
            relist(tap);
        } else
            pushq((Cell)tap, timeeventq);
    }
}

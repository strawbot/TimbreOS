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

static void putTe(TimeEvent* te);

// time actions
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

static bool time_ready = true;

static void checktimeevents() {
    TimeEvent* before = &timeeventList;
    TimeEvent* te;

    while ((te = before->link) != 0) {
        if (checkTimeout(&te->to)) {
            if (!te->action.persist) {
                before->link = te->link;
                putTe(te);
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
    if (te->state == TimeEvent::TE_FREE) {
        te->state = TimeEvent::TE_USED;
        te->link = timeeventList.link;
        timeeventList.link = te;
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
        TimeEvent* te = &timeevents[i];
        te->link = &timeevents[i + 1];
        te->state = TimeEvent::TE_FREE;
        te->index = i;
        te->tag = 0;
    }
    timeevents[TOTAL_TE - 1].link = 0;
    nameMachine(timeevent_IRQ);
    nameMachine(checktimeevents);
}

static TimeEvent* getTe() {
    TimeEvent* te = timeevents[0].link;

    if (te) {
        timeevents[0].link = te->link;
        te->tag++;
        return te;
    }
    while (true);
    print("\nERROR in getTe: No more timeevents left.");
    return timeevents;
}

static void putTe(TimeEvent* te) {
    te->tag++;
    te->state = TimeEvent::TE_FREE;
    te->link = timeevents[0].link;
    timeevents[0].link = te;
}

static TimeEvent * findTe(Long teid) {
    TimeEvent t, *te;
    t.id = teid;
    if (t.index < TOTAL_TE) {
        te = &timeevents[t.index];
        if (te->id == teid)
            return te;
    }
    return nullptr;
}

void stopTe(Long teid) {
    TimeEvent* te = findTe(teid);
    if (te == nullptr) return;

    TimeEvent* before = &timeeventList;
    TimeEvent* tai;

    while ((tai = before->link) != nullptr) {
        if (tai == te) {
            before->link = te->link;
            putTe(te);
            return;
        }
        before = tai;
    }
}

void stopTimeEvent(vector action) {
    TimeEvent* before = &timeeventList;
    TimeEvent* tai;

    while ((tai = before->link) != 0) {
        if (tai->action.object == action) {
            before->link = tai->link;
            putTe(tai);
        } else
            before = tai;
    }
}

void change(Long time, Long id) {
    TimeEvent* te = findTe(id);
    if (te == nullptr) return;
    setTimeout(time, &te->to);
}

Long after(Long time, vector action) {
    TimeEvent* te = getTe();

    setTimeout(time, &te->to);
    te->action.method = (void*)jump;
    te->action.object = (void*)action;
    te->action.persist = false;
    timeevent(te);
    return te->id;
}

Long every(Long time, vector action) {
    Long teid = after(time, action);
    findTe(teid)->action.persist = true;
    return teid;
}

}
// C++
Long after(Long time, void* cpp_obj, unafun action, const char * name) {
    TimeEvent* te = getTe();

    setTimeout(time, &te->to);
    te->action.name = name;
    te->action.method = (void*)action;
    te->action.object = cpp_obj;
    te->action.persist = false;
    timeevent(te);
    return te->id;
}

Long every(Long time, void* cpp_obj, unafun action, const char * name) {
    Long teid = after(time, cpp_obj, action, name);
    findTe(teid)->action.persist = true;
    return teid;
}

void stopTimeEvent(void* cpp_obj, unafun action) {
    TimeEvent* before = &timeeventList;
    TimeEvent* tai;

    while ((tai = before->link) != 0) {
        if (tai->action.object == cpp_obj && tai->action.method == action) {
            before->link = tai->link;
            putTe(tai);
        } else
            before = tai;
    }
}

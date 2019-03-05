// take an action after time
/*
      TimeEvent some_ta;

          after(ta_msecs(25), some_action);
          every(ta_msecs(25), some_action);

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

static void relist(TimeEvent* ta);

// time actions
static QUEUE(30, timeeventq);

static TimeEvent timeeventList;

static Integer time_left(Timeout* timer) {
  Integer elapsed = getTime() - timer->timeset;
  Integer interval = (Integer)timer->timeout;

  return interval - elapsed;
}

void listtimeevents() {
  TimeEvent* ta = &timeeventList;
  print("\nPending timed actions:");
  while ((ta = ta->link) != NULL) {
    print("\n ");
    Cell action = (Cell)&ta->action;
    printHex(action);
    char* name = getMachineName(action);
    if (strlen(name))
      print(name);
    print(" in "), printDec(time_left(&ta->to)), print("ms");
  }
}

static void inlisttimeevents() {
  while (queryq(timeeventq)) {
    TimeEvent* ta = (TimeEvent*)pullq(timeeventq);

    ta->link = timeeventList.link;
    timeeventList.link = ta;
  }
}

static void checktimeevents() {
  TimeEvent* before = &timeeventList;
  TimeEvent* ta;

  while ((ta = before->link) != 0) {
    if (checkTimeout(&ta->to)) {
      if (!ta->action.persist) {
        before->link = ta->link;
        relist(ta);
      } else {
          before = ta;
          repeatTimeout(&ta->to);
      }
      void (*method)(void*) = (void (*)(void*))ta->action.method;
      method(ta->action.object);
    } else
      before = ta;
  }
}

void timeevent_IRQ() {
  inlisttimeevents();
  checktimeevents();
}

__attribute__((weak)) void timeeventError(TimeEvent* ta) {
  printHex((Cell)ta), print(" is already in timeevent queue!");
}

void timeevent(TimeEvent* ta) {
  if (ta->link == LINK_SENTINEL && 0 == scanq((Cell)ta, timeeventq)) {
    ta->link = nullptr;
    pushq((Cell)ta, timeeventq);
  } else
    timeeventError(ta);
}

void after_time(Long t, TimeEvent* ta) {
  setTimeout(t, &ta->to);
  timeevent(ta);
}

void every_time(TimeEvent* ta) {
  if (ta->to.off == true)
    after_time(ta->to.timeout, ta);
  else {
    repeatTimeout(&ta->to);
    timeevent(ta);
  }
}

#define TOTAL_TA 40
static TimeEvent timeevents[TOTAL_TA];

void init_ta() {
  timeeventList.link = nullptr;
  for (int i = 0; i < TOTAL_TA; i++) {
    timeevents[i].link = &timeevents[i + 1];
  }
  timeevents[TOTAL_TA - 1].link = LINK_SENTINEL;
}

TimeEvent* getTa() {
  TimeEvent* ta = timeevents[0].link;

  if (ta != LINK_SENTINEL) {
    timeevents[0].link = ta->link;
    ta->link = LINK_SENTINEL;
    return ta;
  }
  print("\nERROR in getTa: No more timeevents left.");
  return timeevents;
}

static void relist(TimeEvent* ta) {
  ta->link = timeevents[0].link;
  timeevents[0].link = ta;
}

void stopTa(TimeEvent* ta) {
  TimeEvent* before = &timeeventList;
  TimeEvent* tai;

  while ((tai = before->link) != 0) {
    if (tai == ta) {
      before->link = ta->link;
      relist(ta);
      return;
    }
    before = tai;
  }

  for (int n = queryq(timeeventq); n; n--) {
    Cell tap = pullq(timeeventq);
    if (tap == (Cell)ta)
      relist(ta);
    else
      pushq(tap, timeeventq);
  }
}

void stoptimeevent(vector action) {
  TimeEvent* before = &timeeventList;
  TimeEvent* tai;

  while ((tai = before->link) != 0) {
    if (tai->action.method == action) {
      before->link = tai->link;
      relist(tai);
    } else
      before = tai;
  }

  for (int n = queryq(timeeventq); n; n--) {
    TimeEvent* tap = (TimeEvent*)pullq(timeeventq);
    if (tap->action.method == action) {
      relist(tap);
    } else
      pushq((Cell)tap, timeeventq);
  }
}

TimeEvent* after(Long time, vector action) {
  TimeEvent* ta = getTa();

  setTimeout(time, &ta->to);
  ta->action.method = (void*)jump;
  ta->action.object = (void*)action;
  ta->action.persist = false;
  timeevent(ta);
  return ta;
}

TimeEvent* every(Long time, vector action) {
  TimeEvent* ta = after(time, action);
  ta->action.persist = true;
  return ta;
}
}

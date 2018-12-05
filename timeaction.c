// take an action after time
/*
	after(TO_MSECS(25), some_action);

	The interrupt to run the checkTimeouts for all the after's is independant
	of the timeout times. The timeout times are based on BURTC timer. The
	RTC timer is how often the timeouts are checked. If there is one timeout
	and it is for 1 second, then the RTC should be running at least twice that
	rate to give it good accuracy or 500mS.  If a timeout is 10ms, then the
	RTC should run at 5mS. At some point, the interrupt will run too often
	and all the interrupt time will be spent checking timeouts hogging all
	the CPU time. So there should be a lower bound on the RTC setting.

	A simple system sets RTC time to the minimum value and then everything
	just uses that. But this is not the most efficient system. This can be
	obtained dynamically changing the RTC interrupt rate. To make this easier,
	there should be multiple lists. One for each timeouts that fall into the
	classes: > 10 ms, 100 ms, 1 S. When an after or timeout is queued to be
	listed, it is sorted to the list of its type. When a list is empty, then
	the RTC counter can be set to a different time interval. When running all
	three lists, the 10ms list is checked each interrupt. The 100ms list is
	checked each 10 interrupts and the 1S list is checked each 100 interrupts.
*/

#include "queue.h"
#include "machines.h"
#include "timeaction.h"
#include "printers.h"
static QUEUE(30, timeactionq);

static TimeAction timeactionList = {NULL};

static void inlistTimeActions() {
	while (queryq(timeactionq)) {
		TimeAction * ta = (TimeAction *)pullq(timeactionq);

		ta->link = timeactionList.link;
		timeactionList.link = ta;
	}
}

static void checkTimeActions() {
	TimeAction * before = &timeactionList;
	TimeAction * ta;

	while ((ta = before->link) != 0) {
		if (checkTimeout(&ta->to)) {
			before->link = ta->link;
            ta->link = LINK_SENTINEL;
			now(ta->action);
		} else
			before = ta;
	}
}

void timeaction_IRQ() {
	inlistTimeActions();
	checkTimeActions();
}

__attribute__ ((weak)) void timeActionError(TimeAction * ta) {
    print(ta->name), print(" is already in ta queue!");
}

void timeaction(TimeAction * ta) {
    if (ta->link == LINK_SENTINEL) {
        ta->link = NULL;
	    pushq((Cell)ta, timeactionq);
    } else
        timeActionError(ta);
}

void after_time(Long t, TimeAction * ta) {
    setTimeout(t, &ta->to);
    timeaction(ta);
}

void every_time(TimeAction * ta) {
	if (ta->to.off == true)
	    setTimeout(ta->to.timeout, &ta->to);
	else
	    repeatTimeout(&ta->to);
	timeaction(ta); 
}

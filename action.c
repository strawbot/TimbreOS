// take an action after time or event
/*
	  after(ta_msecs(25), some_action);
	  every(ta_msecs(25), some_action);

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
	
	after will run an action after some time while every is similar the
	difference is that "every" is periodic from the first time the action
	executes whereas "after" will run after the time but there may be some
	other time in there as well.
	
	  when(some_event, some_action);
	
	for events, they have a local queue which gets loaded up with machines that
	subscribe to the event. When the event happens, all the queued machines get run.
*/

#include "machines.h"
#include "action.h"
#include "printers.h"

// time actions
static QUEUE(30, timeactionq);

static TimeAction timeactionList = {NULL};

static Integer time_left(Timeout * timer) {
	Integer elapsed = getTime() - timer->timeset;
	Integer interval = (Integer)timer->timeout;

	return interval - elapsed;
}

void listTimeActions() {
    TimeAction * ta = &timeactionList;
    print("\nPending timed actions:");
    while ((ta = ta->link) != NULL)
        print("\n "), print(ta->name), print(" in "),  printDec(time_left(&ta->to)), print("ms");
}

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
			next(ta->action);
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
	    after_time(ta->to.timeout, ta);
	else {
	    repeatTimeout(&ta->to);
	    timeaction(ta); 
    }
}

// event actions
void when(Qtype * actionq, vector action) {
    if (leftq(actionq))
        pushq((Cell)action, actionq);
    else {
    	char * name;
        print("\nError: actionq full!  ");
        if ((name = getMachineName((Cell)action)) != NULL)
            print("Trying to actionize:"), print(name);
    }
}

void runActions(Qtype * actionq) {
    for (int i = queryq(actionq); i; i--) {
        later((vector)q(actionq));
        rotateq(actionq, 1);
    }
}

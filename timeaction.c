// take an action after time
/*
	after(TO_MSECS(25), some_action);
*/

#include "queue.h"
#include "machines.h"
#include "timeaction.h"

static QUEUE(30, timeactionq);

static struct TimeAction * timeactionList = NULL;

static void inlistTimeActions() {
	while (queryq(timeactionq)) {
		TimeAction * ta = (TimeAction *)pullq(timeactionq);
		ta->link = timeactionList;
		timeactionList = ta;
	}
}

static void checkTimeActions() {
	TimeAction ** before = &timeactionList;
	TimeAction * ta = timeactionList;

	while (ta) {
		if (checkTimeout(&ta->to)) {
			now(ta->action);
			(*before)->link = ta->link;
		} else
			before = &ta;
		ta = ta->link;
	}
}

void timeaction_IRQ() {
	inlistTimeActions();
	checkTimeActions();
}

void doLater(Long t, TimeAction * ta) {
	setTimeout(t, &ta->to);
	pushq((Cell)ta, timeactionq);
}


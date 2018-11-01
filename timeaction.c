// take an action after time
/*
	after(TO_MSECS(25), some_action);
*/

#include "queue.h"
#include "machines.h"
#include "timeaction.h"

static QUEUE(30, timeactionq);

static struct TimeAction timeactionList = {NULL};

static void inlistTimeActions() {
	while (queryq(timeactionq)) {
		TimeAction * ta = (TimeAction *)pullq(timeactionq);

		ta->link = timeactionList.link;
		timeactionList.link = ta;
	}
}

static void checkTimeActions() {
	TimeAction * before = &timeactionList;
	TimeAction * ta = before->link;

	while (ta) {
		if (checkTimeout(&ta->to)) {
			before->link = ta->link;
			now(ta->action);
		} else
			before = ta;
		ta = before->link;
	}
}

void timeaction_IRQ() {
	inlistTimeActions();
	checkTimeActions();
}

void timeaction(TimeAction * ta) {
	pushq((Cell)ta, timeactionq);
}

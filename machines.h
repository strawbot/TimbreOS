// for machines  Rob Chapman  Jan 22, 2010

#ifndef _MACHINES_H_
#define _MACHINES_H_

#include "queue.h"
#include "timeEvent.h"
#include "event.h"

void activate (vector machine);
void next (vector machine);
#define later(m)    activate(m)

void activateMachineOnce(vector machine);
void deactivate(vector machine);
void runMachines(void);
void initMachines(void);
void listm(void);
void listq(Qtype * q);
void machineName(vector machine, const char * name);
void activateOnceNamed(vector machine, const char *name);
void showMachineName(Cell x);
vector getMachine(char * name);
char * getMachineName(Cell x);
void machineRun(vector m);
void runMachineqs();
void run_till_done();
void run_slice();

#define activateOnce(m)	activateOnceNamed(m, #m)
#define nameMachine(m) machineName(m, #m)

#define MACHINES (50)
#define ACTIONS (MACHINES)
#define EVENTS (MACHINES)

#ifndef ATOMIC_SECTION_ENTER // define if used in interrupts
	#define ATOMIC_SECTION_ENTER
	#define ATOMIC_SECTION_LEAVE
#endif

/*
void noop(void);

// different machine mechanism; turn a machine on and off
#define machine(machine)	vector machine##_machine = noop;
#define activate(machine)	machine##_machine = machine;
#define deactivate(machine)	machine##_machine = noop;
#define enact(machine)		machine##_machine();

	in machine.c
	
	void machines(void)
	{
		enact(machine1);
		enact(machine2);
		...
	}
	
	in file1.c
	machine(machine1); // declare
		
		activate(machine1); // turn on
		...
		deactivate(machine1); // turn off
*/

#endif

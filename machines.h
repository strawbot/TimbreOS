// for machines  Rob Chapman  Jan 22, 2010

#ifndef _MACHINES_H_
#define _MACHINES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "timeEvent.h"

#define activate(m)    later(m)

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

#ifdef __cplusplus

extern "C++" void later(void* object, unafun unary, const char * name = "");
extern "C"   void later (vector machine);
extern "C"   void next (vector machine);

#else

void later (vector machine);
void next (vector machine);

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

#ifdef __cplusplus
}
#endif

#endif

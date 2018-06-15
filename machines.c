// Machines for tasks  Rob Chapman  Dec 30, 2009

// use compiler flags to select atomic protection or not
// and for tracking under/overflows

// add depth detector for runMachines. Each time it is entered, depth increases by 1
// each time it is finished, depth decreases by 1
// add protection to runMachines so it can tell if it is recursive

#include "machines.h"
#include "cli.h"
#include <string.h>

QUEUE(MACHINES, machineq);
Byte mmoverflow = 0, mmunderflow = 0;

void activate(vector Machine)
{
	ATOMIC_SECTION_ENTER;
	if (leftq(machineq) == 0)
		mmoverflow++;
	else
		pushq((Cell)Machine, machineq);
	ATOMIC_SECTION_LEAVE;
}

void deactivate(vector machine) // remove a machine from queue
{
	Byte n;

	ATOMIC_SECTION_ENTER;
	n = (Byte)queryq(machineq);
	while(n--)
	{
		Cell m = pullq(machineq);
		if (m != (Cell)machine)
			pushq(m, machineq);
	}
	ATOMIC_SECTION_LEAVE;
}

void activateMachineOnce(vector machine) // only have one occurrance of a machine running
{
	deactivate(machine);
	activate(machine);
}

static Long runDepth = 0;

void runMachines(void) // run all machines
{
	Byte n = (Byte)queryq(machineq);

	runDepth++;
	while(n--)
	{
		vector machine;
		if (queryq(machineq) == 0)
		{
			mmunderflow++;
			break;
		}
		safe(machine = (vector)pullq(machineq));
		machineRun(machine);
	}
	runDepth--;
}

// viewers
#include "dictionary.h"
#include "printers.h"

HASHDICT(HASH9, macnames); // keep track of machine names
HASHDICT(HASH9, mactimes); // keep track of machine max execution times

void initMachineStats() {
    emptyDict(&macnames);
    emptyDict(&mactimes);
}

void zeroMachineTimes() {
    for (Short i=0; i<mactimes.capacity; i++)
        mactimes.adjunct[i] = 0;
}

void machineName(vector machine, const char * name) // give name to machine
{
	dictAddKey((Cell)machine, &macnames);
	dictAddKey((Cell)machine, &mactimes);
	*dictAdjunctKey((Cell)machine, &macnames) = (Cell)name;
}

void activateOnceNamed(vector machine, const char * name)
{
	activateMachineOnce(machine);
	machineName(machine, name);
}

char * getMachineName(Cell x) {
	char * name = *(char **)dictAdjunctKey(x, &macnames);

	if (name)
		return name;
    return (char *)"";
}

void showMachineName(Cell x)
{
	print("\n"), printHex(x), print(": ");
    print(getMachineName(x));
}

vector getMachine(char * name) // return address of named machine
{
	for (Short i=0; i<macnames.capacity; i++)
		if (macnames.adjunct[i] != 0)
            if (strcmp((char *)macnames.adjunct[i], name) == 0)
			    return (vector)(Cell)macnames.table[i];
	return NULL;
}

void listq(Qtype *q) // list q items
{
	Byte n;

	n = (Byte)queryq(q);
	{
		Cell l[n];

		ATOMIC_SECTION_ENTER;
		for (Byte i=0; i<n; i++) {
			l[i] = pullq(q);
			pushq(l[i], q);
		}
		ATOMIC_SECTION_LEAVE;
		for (Byte i=0; i<n; i++)
			showMachineName(l[i]);
	}
}

void listMachines(void)
{
	print("\nDepth: "), printDec(runDepth);
	listq(machineq);
	if (mmunderflow)
		print("\nmunderflows: "), printDec(mmunderflow);
	if (mmoverflow)
		print("\nmoverflows: "), printDec(mmoverflow);
	dotPrompt();
}

void listm(void) // list machine statuses
{
	activate(listMachines);
}

void initMachines(void)
{
	zeroq(machineq);
	initMachineStats();
}

void killMachine() {
	vector mac = getMachine((char *)parseWord(0));

	if (mac)
		deactivate(mac);
	else
		print("Machine not available.\n");
}

void activateMachine() {
	vector mac = getMachine((char *)parseWord(0));

	if (mac)
		activate(mac);
	else
		print("Machine not available.\n");
}

// Machine cycle timer
#include "codeStats.c"
#include "printers.h"
#include <stdlib.h>

void machineRun(vector m) {
	Cell * stat = dictAdjunctKey((Cell)m, &mactimes);
	if (stat) {
		int time = (int)getTicks();
		m();
        time = (int)getTicks() - time;
        if (time > (int)*stat)
            *stat = (Cell)time;
	} else
		m();
}

static int indexCompare(const void *a,const void *b) {
	Short *x = (Short *) a;
	Short *y = (Short *) b;
	return mactimes.adjunct[*y] - mactimes.adjunct[*x];
}

void machineStats(void)
{
	Short indexes[mactimes.capacity];
	Short j=0;

	for (Short i=0; i<mactimes.capacity; i++)
		if (mactimes.adjunct[i] != 0)
			indexes[j++] = i;
	
	qsort(indexes, j, sizeof(Short), indexCompare);
	
	for (Short i=0; i<j; i++) {
		Cell machine = (Cell)mactimes.table[indexes[i]];
		char * name = (char *)macnames.adjunct[indexes[i]];
		printCr();
		Cell time = mactimes.adjunct[indexes[i]];
		if (CONVERT_TO_US(time) > 9999)
			dotnb(7, 6, CONVERT_TO_MS(time), 10), print(" ms  ");
		else
			dotnb(7, 6, CONVERT_TO_US(time), 10), print(" us  ");
		if (name)
			print(name);
		else
			printHex(machine);
	}
}


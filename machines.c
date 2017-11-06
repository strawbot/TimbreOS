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
	pushq((Cell)Machine, machineq);
	if (queryq(machineq) == 0)
		mmoverflow++;
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

static void monitor(vector m);

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
		monitor(machine);
	}
	runDepth--;
}

// viewers
#include "dictionary.h"
#include "printers.h"

HASHDICT(HASH8, macnames); // keep track of machine names
HASHDICT(HASH8, mactimes); // keep track of machine max execution times

void initMachineStats() {
    emptyDict(&macnames);
    emptyDict(&mactimes);
}

void zeroMachineTimes() {
	memset(mactimes.adjunct, 0, sizeof(mactimes.adjunct));
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

// vector getMachine(char * name) // return address of named machine
// {
// 	Cell i;
// 
// 	i = queryq(machinenameq)/2;
// 	while(i--) {
// 		Cell m = pullq(machinenameq);
// 		Cell n = pullq(machinenameq);
// 		pushq(m, machinenameq);
// 		pushq(n, machinenameq);
// 		if (strcmp((char *)n,name) == 0)
// 			return (vector)m;
// 	}
// 	return NULL;
// }
// 
void listq(Qtype *q) // list q items
{
	Byte n;

	n = (Byte)queryq(q);
	{
		Cell l[MACHINES]; // must be greater than n

		ATOMIC_SECTION_ENTER;
		for (Byte i=0; i<n; i++)
		{
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
// 	vector mac = getMachine((char *)parseWord(0));
// 
// 	if (mac)
// 		deactivate(mac);
// 	else
// 		print("Machine not running.\n");
}

// Machine cycle timer
#include "timestamp.h"
#include "printers.h"

static void monitor(vector m) {
	Cell * stat = dictAdjunctKey((Cell)m, &mactimes);
	if (stat) {
		Cell time = getTime();
		m();
		time = getTime() - time;
		if (*stat < time)
			*stat = time;
	} else
		m();
}

void machineStats(void)
{
	for (Short i=0; i<macnames.capacity; i++)
		if (macnames.adjunct[i] != 0) {
			printCr(), print((char *)macnames.adjunct[i]), print(": ");
			printDec(mactimes.adjunct[i]), print("ms");
		}
}


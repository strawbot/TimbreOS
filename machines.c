// Machines for tasks  Rob Chapman  Dec 30, 2009

// use compiler flags to select atomic protection or not
// and for tracking under/overflows

// add depth detector for runMachines. Each time it is entered, depth increases by 1
// each time it is finished, depth decreases by 1
// add protection to runMachines so it can tell if it is recursive

#include "machines.h"
#include "cli.h"

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
		machine();
	}
	runDepth--;
}

// viewers
#if 1
#include "printers.h"
QUEUE(MACHINES * 2, machinenameq);

void machineName(vector machine, const char * name) // give name to machine
{
	Cell i;

	i = queryq(machinenameq)/2U;
	while(i--)
	{
		Cell m = pullq(machinenameq);
		Cell n = pullq(machinenameq);
		if (m != (Cell)machine)
			pushq(m, machinenameq), pushq(n, machinenameq);
	}
	pushq((Cell)machine, machinenameq);
	pushq((Cell)name, machinenameq);
}

void activateOnceNamed(vector machine, const char * name)
{
	activateMachineOnce(machine);
	machineName(machine, name);
}

void showMachineName(Cell x)
{
	Cell i;

	print("\n"), printHex(x), print(": ");
	i = queryq(machinenameq)/2;
	while(i--) {
		Cell m = pullq(machinenameq);
		Cell n = pullq(machinenameq);
		pushq(m, machinenameq);
		pushq(n, machinenameq);
		if (m == x) {
			print((char *)n);
			return;
		}
	}
}

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
#endif

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

void resetMachineMonitor(void);

void initMachines(void)
{
	zeroq(machineq);
	zeroq(machinenameq);
	activate(resetMachineMonitor);
}

// Machine cycle timer  Robert Chapman  Jan 16, 2015
// runs as a machine and keeps track of min, avg and max times between runs
#include "timestamp.h"

#define N 100

static Long minLoop, maxLoop, sumLoop, countLoop;
static Long lastTime;
static QUEUE(N, sumq);

static void machineMonitor(void);
//Keeps statistics on minimum and maximum run time for a queue of machines
static void machineMonitor(void)
{
	Long thisTime = getTime();
	
	if (lastTime)
	{
		Long span = thisTime - lastTime;
		
		if (span < minLoop)
			minLoop = span;
		else if (span > maxLoop)
			maxLoop = span;
		
		if (queryq(sumq) == N)
			sumLoop -= pullq(sumq);

		pushq(span, sumq);
		sumLoop += span;
	}
	lastTime = thisTime;
	countLoop++;
	activate(machineMonitor);
}

// CLI
#include "printers.h"

void machineStats(void);
void machineStats(void)
{
	print("\nLoop times (ms). Min: ");
	printDec(minLoop);
	print("  Max: ");
	printDec(maxLoop);
	print("  Average: ");
	if (queryq(sumq))
		printDec(sumLoop/queryq(sumq));
	else
		print("no sum ");
	print("  #loops: ");
	printDec(countLoop);
}

void resetMachineMonitor(void)
{
	minLoop = 1000000000;
	maxLoop = 0;
	sumLoop = 0;
	countLoop = 0;
	zeroq(sumq);
	lastTime = 0;
	activateOnce(machineMonitor);
}

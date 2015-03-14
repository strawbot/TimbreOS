// Machines for tasks  Rob Chapman  Dec 30, 2009

// use compiler flags to select atomic protection or not
// and for tracking under/overflows

#include "machines.h"
#include "queue.h"
#include "kernel.h"
#include "library.h"

QUEUE(MACHINES, machineq);
Byte mmoverflow, mmunderflow;

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
	Cell m;

	ATOMIC_SECTION_ENTER;
	n = (Byte)queryq(machineq);
	while(n--)
	{
		m = pullq(machineq);
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

void runMachines(void) // run all machines
{
	Byte n = (Byte)queryq(machineq);
	vector machine;

	while(n--)
	{
		if (queryq(machineq) == 0)
		{
			mmunderflow++;
			break;
		}
		safe(machine = (vector)pullq(machineq));
		machine();
	}
}

// viewers
#if 1
#include "printers.h"
QUEUE(MACHINES * 2, machinenameq);

void activateOnceNamed(vector machine, const char * name)
{
	Cell m,n,i;

	activateMachineOnce(machine);
	i = queryq(machinenameq)/2U;
	while(i--)
	{
		m = pullq(machinenameq);
		n = pullq(machinenameq);
		if (m != (Cell)machine)
			pushq(m, machinenameq), pushq(n, machinenameq);
	}
	pushq((Cell)machine, machinenameq);
	pushq((Cell)name, machinenameq);
}

void showMachineName(Cell x)
{
	Cell m,n,i;

	print("\n"), printHex(x), print(": ");
	i = queryq(machinenameq)/2;
	while(i--) {
		m = pullq(machinenameq);
		n = pullq(machinenameq);
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
	Byte n,i;

	n = (Byte)queryq(q);
	{
		Cell l[MACHINES]; // must be greater than n

		ATOMIC_SECTION_ENTER;
		for (i=0; i<n; i++)
		{
			l[i] = pullq(q);
			pushq(l[i], q);
		}
		ATOMIC_SECTION_LEAVE;
		for (i=0; i<n; i++)
			showMachineName(l[i]);
	}
}
#endif

void listm(void) // list machine statuses
{
	listq(machineq);
	if (mmunderflow)
		print("\nmunderflows"), printDec(mmunderflow);
	if (mmoverflow)
		print("\nmoverflows"), printDec(mmoverflow);
	print("\n");
}

void resetMachineMonitor(void);

void initMachines(void)
{
	zeroq(machineq);
	zeroq(machinenameq);
	resetMachineMonitor();
}

// Machine cycle timer  Robert Chapman  Jan 16, 2015
// runs as a machine and keeps track of min, avg and max times between runs
#include "timestamp.h"

#define N 100

static Long minLoop, maxLoop, sumLoop, countLoop;
static Long lastTime;
static QUEUE(N, sumq);

static void machineMonitor(void);
static void machineMonitor(void)
{
	Long span, thisTime = getTime();
	
	if (lastTime)
	{
		span = thisTime - lastTime;
		
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

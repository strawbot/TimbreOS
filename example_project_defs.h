// Sample project definitions file - copy this to your project and edit as needed
#ifndef PROJECT_DEFS_H_
#define PROJECT_DEFS_H_

#include "ttypes.h"
// board specific includes:

// bigger buffer for accepting long hexscii sequences
#define CLI_PARAMETERS

#define CLI_TITLE "TimbreOS\n"

#define DCELLS 20  // number of data stack cells
#define RCELLS 20  // number of return stack cells
#define LINE_LENGTH 400 // number of characters allowed in tib
#define EMITQ_SIZE 400
#define KEYQ_SIZE 400
#define PAD_SIZE 20
#define PROMPTSTRING "Timbre: "
#define CUSHION LINE_LENGTH // how much space to maintain for HERE
#define HERE_SPACE 1000 // small here space
#define OUTPUT_BLOCKED output() // deal with by running machines
#define OUTPUT_FLUSH output()
#define FLOAT_SUPPORT true
#define NAN (__builtin_nanf(""))

// define these to deal with output blocking and flushing
void output_flush();
void output();

// UTC and time event clocks
#define CLOCK_MHZ 84u
#define ONE_SECOND (10000)	// for UTC 100us res
#define TE_SECOND ONE_SECOND    // for Delta timer
#define get_utc() 0  // utc in seconds

// Hi res time measurements
// CPU clock ticks
#define sysTicks()  (Long)(DWT->CYCCNT)

#define SYS_TO_NS(n) ((unsigned long long)(n)*1000/CLOCK_MHZ)
#define SYS_TO_US(n) ((n)/CLOCK_MHZ)
#define SYS_TO_MS(n) ((unsigned long long)(n)/(CLOCK_MHZ*1000))
#define US_TO_SYS(n)	 ((n)*CLOCK_MHZ)

#define IN_INTERRUPT() 	(SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk)

// cortex M3/M4 critical section support
#define ENTER_REGION()                                          \
	{                                                           \
		uint32_t primask_bit = __get_PRIMASK();					\
		__disable_irq();

#define LEAVE_REGION()                                          \
		  __set_PRIMASK(primask_bit);                           \
	}

#define ENTER_SAFE_REGION() ENTER_REGION()
#define LEAVE_SAFE_REGION() LEAVE_REGION()

// Record event parameters
#define NUM_ACTIONS 80
#define NUM_TE 80

#define N_EVENTS 100
#define FIRST_EVENT (const char *)secs(5)

// define space for action stats
#define TEA_TABLE HASH8

// black hole reasons - addendum
#define DMA_OVERBOOKED 7

#endif

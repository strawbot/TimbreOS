// Generic stats counter for a file  Robert Chapman  Dec 15, 2017
//#include "em_timer.h"

// 14 MHz clock ticks; wraps after 5 minutes; 71.4 ns resolution
#ifndef CLOCK_MHZ
  #define CLOCK_MHZ 14
#endif

// ARM Cortex-M3 extended trace macrocell registers: Data Watchpoint and Trace (DWT)
//#define getTicks() (DWT->CYCCNT) // cortex-M3 cycle counter from ETM
#define getTicks() 0

#define CONVERT_TO_NS(n) ((long long)(n)*1000/CLOCK_MHZ)
#define CONVERT_TO_US(n) ((n)/CLOCK_MHZ)
#define CONVERT_TO_MS(n) ((long long)(n)/(CLOCK_MHZ*1000))
#define US_TO_TICKS(n)	 ((n)*CLOCK_MHZ)

#ifdef FOR_EACH_STAT
/*
 use to increment stat counters within code to find out which paths are being used
 at run time.
 To use, define a list like this:
    #define FOR_EACH_STAT(F) \
        F(BadSync) \
        F(GoodFrame) \
        F(BadCheckSum) \
        F(RxTimeout) \
        F(BadLength) \
 and then include this file

 To see your stats invoke the function printStats() which will also clear the stats.
 Only non zero stats are printed.
 */

#include "printers.h"
#include <string.h>

#define ENUM_STAT(stat) stat,

enum{FOR_EACH_STAT(ENUM_STAT) NUM_STATS};

static Cell statCounts[NUM_STATS] = {0};

#define PRINT_STAT(stat) \
	if (statCounts[stat]) \
		print("\n"#stat": "), printDec(statCounts[stat]);

inline static void printStats() {
    FOR_EACH_STAT(PRINT_STAT)
    memset(statCounts, 0, sizeof(statCounts));
}

// use these macros to use the stats
#define getStat(stat) statCounts[stat]
#define setStat(stat, val) statCounts[stat] = val
#define incStat(stat) statCounts[stat]++
#define maxStat(n, stat) if ((n) > statCounts[stat]) statCounts[stat] = n

#else
typedef int make_iso_compilers_happy;
#endif

// Time measures in microseconds
#define startUS() Cell startingTime = getTicks()
#define endUS() CONVERT_TO_US(getTicks() - startingTime)
#define maxUS(stat) maxStat(endUS(), stat)
#define waitUS(n)  startUS(); while(US_TO_TICKS(n) > getTicks() - startingTime)

// Time measures in milliseconds
#define startMS() startUS()
#define endMS() CONVERT_TO_MS(getTicks() - startingTime)
#define maxMS(stat) maxStat(endMS(), stat)

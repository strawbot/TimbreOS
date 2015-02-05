// Header file for timestamp  Rob Chapman  Mar 14, 2011

#include "bktypes.h"

void oneMillisecondTick(void);
void initTimeStamp(void);
Long getTime(void);
void showTime(void);
void startTime(void);
void sdotms(Long time);
void endTime(void);

#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#define LOCALTIME_TICK 1 	// milliseconds per tick
#define ACKNOWLEDGE_TICK // PCSR0 = PCSR0;

#endif

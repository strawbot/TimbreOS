#ifndef CLOCKS_H_
#define CLOCKS_H_

#include "ttypes.h"
#include "project_defs.h"
#include <time.h>

// Libc-free UTC date/time math (no mktime, gmtime, sscanf)
Long timestamp_to_utc(const char *ts);    // parse __TIMESTAMP__ → Unix epoch
void epoch_to_tm(Long utc, struct tm *t); // epoch → struct tm
Long tm_to_epoch(const struct tm *t);     // struct tm → epoch

// Clock API (provided by TimbreOS/clocks.c or the board's clocks.c)
Long get_ticks(void);
void set_delta_alarm(Long t);
void delta_alarm(void);
void over_due(void);
void micro_sleep(void);
void init_clocks(void);
void show_timer(void);
void print_build_banner(void);
void blink_leds(void);

#endif /* CLOCKS_H_ */

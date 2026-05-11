// clocks.c — board-independent TimbreOS clock utilities
//
// ── WHAT LIVES HERE ───────────────────────────────────────────────────────
//  • Libc-free UTC date math: timestamp_to_utc / epoch_to_tm / tm_to_epoch
//  • Portable stubs:          over_due, micro_sleep, print_build_banner
//  • Common heartbeat blink:  blink_leds  (double-blink via LED macros)
//  • Common diagnostic:       show_timer
//  • STM LL timer functions:  set_delta_alarm, delta_alarm, get_ticks
//  • Common init:             init_clocks  (DWT + SysTick + timer start)
//
// ── BOARD WIRING (add to the board's project_defs.h) ─────────────────────
//
//  Required for STM LL timer functions and init_clocks:
//    CLOCK_DELTA_TIM      timer peripheral for the one-shot delta alarm
//    CLOCK_TICK_TIM       timer peripheral for the free-running tick counter
//    CLOCK_HW_INIT()      board-specific timer peripheral setup called from
//                         init_clocks() after DWT/SysTick are configured
//
//  Required unless CLOCK_HAS_BLINK is defined:
//    CLOCK_LED_ON()       turn the heartbeat LED on
//    CLOCK_LED_OFF()      turn the heartbeat LED off
//
//  Optional:
//    CLOCK_DELTA_MAX      maximum ticks for one alarm period (default 0xFFFF)
//    CLOCK_INIT_UTC()     called at end of init_clocks() to seed the RTC
//                         e.g. set_utc(timestamp_to_utc(__TIMESTAMP__))
//
//  Opt-out guards (define to supply a board-specific replacement):
//    CLOCK_HAS_BLINK      board provides blink_leds()
//    CLOCK_HAS_INIT       board provides init_clocks()
//    CLOCK_HAS_TICKS      board provides get_ticks()
//    CLOCK_HAS_DELTA      board provides set_delta_alarm() and delta_alarm()
//    CLOCK_HAS_SHOW       board provides show_timer()

#include "project_defs.h"
#include "tea.h"
#include "printers.h"
#include <time.h>

// ── Libc-free UTC date/time math ─────────────────────────────────────────
//
// __TIMESTAMP__ has the fixed format "Www Mmm DD HH:MM:SS YYYY".
// Howard Hinnant's civil calendar algorithms; no mktime/sscanf/gmtime.
// Avoids bare-metal newlib hangs from malloc / _sbrk retries in those calls.

Long timestamp_to_utc(const char *ts) {
    static const char mon_names[12][3] = {
        {'J','a','n'},{'F','e','b'},{'M','a','r'},{'A','p','r'},
        {'M','a','y'},{'J','u','n'},{'J','u','l'},{'A','u','g'},
        {'S','e','p'},{'O','c','t'},{'N','o','v'},{'D','e','c'}
    };
    #define DIG(c) ((c) >= '0' && (c) <= '9' ? (c) - '0' : 0)
    // Offsets: "Www Mmm DD HH:MM:SS YYYY"
    //           0   4   8  11 14 17 20
    int day  = DIG(ts[8])  * 10 + DIG(ts[9]);
    int hour = DIG(ts[11]) * 10 + DIG(ts[12]);
    int min  = DIG(ts[14]) * 10 + DIG(ts[15]);
    int sec  = DIG(ts[17]) * 10 + DIG(ts[18]);
    int year = DIG(ts[20]) * 1000 + DIG(ts[21]) * 100
             + DIG(ts[22]) * 10   + DIG(ts[23]);
    #undef DIG
    int month = 1;
    for (int i = 0; i < 12; i++) {
        if (ts[4] == mon_names[i][0] &&
            ts[5] == mon_names[i][1] &&
            ts[6] == mon_names[i][2]) { month = i + 1; break; }
    }
    int y   = (month <= 2) ? (year - 1) : year;
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153u * (unsigned)(month + (month > 2 ? -3 : 9)) + 2u) / 5u
                 + (unsigned)(day - 1);
    unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    long days = (long)era * 146097L + (long)doe - 719468L;
    return (Long)(days * 86400L + (long)hour * 3600L + (long)min * 60L + (long)sec);
}

// Epoch (Unix UTC seconds) → struct tm. Uses Hinnant's civil_from_days.
void epoch_to_tm(Long utc, struct tm *t) {
    long days = (long)utc / 86400L;
    long sod  = (long)utc - days * 86400L;
    if (sod < 0) { sod += 86400L; days -= 1L; }
    t->tm_hour = (int)(sod / 3600L);
    t->tm_min  = (int)((sod % 3600L) / 60L);
    t->tm_sec  = (int)(sod % 60L);
    long wd = ((days % 7L) + 4L) % 7L;
    if (wd < 0) wd += 7L;
    t->tm_wday = (int)wd;
    days += 719468L;
    long era = (days >= 0 ? days : days - 146096L) / 146097L;
    unsigned doe = (unsigned)(days - era * 146097L);
    unsigned yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    long y       = (long)yoe + era * 400L;
    unsigned doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    unsigned mp  = (5u * doy + 2u) / 153u;
    unsigned d   = doy - (153u * mp + 2u) / 5u + 1u;
    unsigned m   = mp < 10u ? mp + 3u : mp - 9u;
    int year     = (int)(y + (m <= 2 ? 1 : 0));
    t->tm_year  = year - 1900;
    t->tm_mon   = (int)m - 1;
    t->tm_mday  = (int)d;
    t->tm_yday  = 0;
    t->tm_isdst = 0;
}

// struct tm → epoch. Uses Hinnant's days_from_civil.
Long tm_to_epoch(const struct tm *t) {
    int year  = t->tm_year + 1900;
    int month = t->tm_mon  + 1;
    int y   = (month <= 2) ? (year - 1) : year;
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153u * (unsigned)(month + (month > 2 ? -3 : 9)) + 2u) / 5u
                 + (unsigned)(t->tm_mday - 1);
    unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    long days    = (long)era * 146097L + (long)doe - 719468L;
    return (Long)(days * 86400L + (long)t->tm_hour * 3600L
                + (long)t->tm_min * 60L + (long)t->tm_sec);
}

// ── Board-independent stubs ───────────────────────────────────────────────

void over_due(void) {}

// WFI is ARM Cortex-M. Use direct asm so no CMSIS header is required.
void micro_sleep(void) {
#if defined(__arm__) || defined(__thumb__)
    __asm volatile ("wfi" ::: "memory");
#endif
}

void print_build_banner(void) {
    print("\nBuilt: ");
    print(__TIMESTAMP__);
}

// ── Heartbeat double-blink ────────────────────────────────────────────────
// Requires CLOCK_LED_ON() and CLOCK_LED_OFF() from project_defs.h.
// Define CLOCK_HAS_BLINK to supply a custom blink_leds() in the board's
// clocks.c (e.g. Discovery's 4-LED colour wheel).

#ifndef CLOCK_HAS_BLINK
#define BLINK_ON_MS 2
void blink_leds(void) {
    Long t;
    static enum { B1, G1, B2, G2 } phase = G2;
    switch (phase) {
    case B1: CLOCK_LED_ON();  phase = G1; t = msec(BLINK_ON_MS);       break;
    case G1: CLOCK_LED_OFF(); phase = B2; t = msec(200 - BLINK_ON_MS); break;
    case B2: CLOCK_LED_ON();  phase = G2; t = msec(BLINK_ON_MS);       break;
    default:
    case G2: CLOCK_LED_OFF(); phase = B1; t = msec(800 - BLINK_ON_MS); break;
    }
    in(t, blink_leds);
}
#endif /* CLOCK_HAS_BLINK */

// ── STM LL timer functions ────────────────────────────────────────────────
// Compiled only when CLOCK_DELTA_TIM / CLOCK_TICK_TIM are defined (i.e. for
// boards using the STM LL timer API).  Boards with a different timer API
// (Nano FSP, TIVA TivaWare) define CLOCK_HAS_DELTA / CLOCK_HAS_TICKS.

#ifndef CLOCK_DELTA_MAX
#define CLOCK_DELTA_MAX 0xFFFFu
#endif

#if defined(CLOCK_DELTA_TIM) && !defined(CLOCK_HAS_DELTA)
void set_delta_alarm(Long t) {
    if ((unsigned long)t > CLOCK_DELTA_MAX) { print("#"); t = (Long)CLOCK_DELTA_MAX; }
    if (t < 1) t = 1;
    LL_TIM_SetAutoReload(CLOCK_DELTA_TIM, (uint32_t)(t - 1));
    LL_TIM_SetCounter(CLOCK_DELTA_TIM, 0);
    LL_TIM_EnableCounter(CLOCK_DELTA_TIM);
}

void delta_alarm(void) {
    LL_TIM_ClearFlag_UPDATE(CLOCK_DELTA_TIM);
    now(*alarmEvent);
}
#endif /* CLOCK_DELTA_TIM && !CLOCK_HAS_DELTA */

#if defined(CLOCK_TICK_TIM) && !defined(CLOCK_HAS_TICKS)
Long get_ticks(void) { return (Long)LL_TIM_GetCounter(CLOCK_TICK_TIM); }
#endif /* CLOCK_TICK_TIM && !CLOCK_HAS_TICKS */

// ── show_timer ────────────────────────────────────────────────────────────
// Basic version: ticks/s, current ticks, UTC epoch.
// Define CLOCK_HAS_SHOW for a board-specific version (e.g. Discovery's
// gmtime/strftime display, or TIVA's uptime seconds).

#ifndef CLOCK_HAS_SHOW
void show_timer(void) {
    print("  ticks/S:"); printDec(ONE_SECOND);
    print("  ticks:");   printDec(get_ticks());
    print("  UTC:");     printDec(get_utc());
}
#endif /* CLOCK_HAS_SHOW */

// ── init_clocks ───────────────────────────────────────────────────────────
// Requires CLOCK_HW_INIT() from project_defs.h for the board's timer setup.
// Optional CLOCK_INIT_UTC() to seed the RTC from __TIMESTAMP__ at boot.
// Define CLOCK_HAS_INIT for a board-specific init_clocks() (Nano FSP, TIVA).

#ifndef CLOCK_INIT_UTC
#define CLOCK_INIT_UTC() do {} while (0)
#endif

#ifndef CLOCK_HAS_INIT
void init_clocks(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
    SysTick->CTRL = 0;
    never(alarmEvent);
    CLOCK_HW_INIT();
    CLOCK_INIT_UTC();
    later(blink_leds);
    namedAction(blink_leds);
}
#endif /* CLOCK_HAS_INIT */

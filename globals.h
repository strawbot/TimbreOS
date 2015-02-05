
/* Virtual Interpreter Machinery */
// Copyright TimbreWorks Engineering 1990-2005

#ifdef G400
register unsigned long *sp asm ("%a3"),*rp asm ("%a2");        /* Stacks */
register void (***ip)() asm ("%a4"),(**tick)() asm ("%a5");     /* Threader */
#else
#ifdef POLLYNOT
register unsigned long *sp asm ("g9"),*rp asm ("g10");        /* Stacks */
register void (***ip)() asm ("g11"),(**tick)() asm ("g12");     /* Threader */
#else
Cell *sp, *rp;  /* Stack pointers */
thread *ip, tick; /* Indirect-threaded code pointers */
#endif
#endif


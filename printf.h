#include "printers.h"

#ifndef PRINTF_H
#define PRINTF_H

#include "printers.h"
#define printf(...)  { char buf[1024]; sprintf(buf, __VA_ARGS__); print(buf); }

#endif
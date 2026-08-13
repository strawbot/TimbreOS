#include "printers.h"

#ifndef PRINTF_H
#define PRINTF_H

#include "printers.h"
#define printf(...)  { char buf[128]; snprintf(buf, 128, __VA_ARGS__); print(buf); }

#endif
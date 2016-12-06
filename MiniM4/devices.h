#include "stm32f4xx_hal.h"
#ifndef _DEVICES_H_
#define _DEVICES_H_

#define SERIAL_UART huart3
#define SFP_UART huart3
#define TIME_BASE htim7

extern UART_HandleTypeDef huart3;

#endif
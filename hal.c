// Hardware interface  Robert Chapman III  Oct 24, 2016

// This file accesses the HAL stuff and gives it a name to be used by the project
// This is under the same set of file includes as the solution and allows the
// TimbreOS to have a different set of includes or directory structure if needed

#include "stm32f4xx_hal.h"
#include "bktypes.h"

// plugs
void greenOn(void)
{
	HAL_GPIO_WritePin(YellowLed_GPIO_Port, YellowLed_Pin, GPIO_PIN_SET);
}

void greenOff(void)
{
	HAL_GPIO_WritePin(YellowLed_GPIO_Port, YellowLed_Pin, GPIO_PIN_RESET);
}

// Use system time reference
Long getTime(void)
{
	return HAL_GetTick();
}

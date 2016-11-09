// Memory interface for CPLD using GPIO  Robert Chapman III  Aug 31, 2015

#include "botkernl.h"
#include "printers.h"
#include "stm32f4xx_hal.h"
#include "memory.h"

// Externals
extern SDRAM_HandleTypeDef hsdram1;

// CPLD
#define writePin(level, port, pin) \
	if (level) GPIO##port->BSRR = GPIO_PIN_##pin; \
	else GPIO##port->BSRR = GPIO_PIN_##pin << 16;

#define readPin(value, port, pin) \
	HAL_GPIO_ReadPin (GPIO##port, GPIO_PIN_##pin) ? value : 0 ;

void writeAddress(Byte address)
{
	writePin (address & 1, A, 13)
	writePin (address & 2, A, 14)
	writePin (address & 4, A, 15)
	writePin (address & 8, B, 0)
	writePin (address & 16, H, 5)
	writePin (address & 32, B, 2)
	writePin (address & 64, B, 3)
	writePin (address & 128, B, 4)
}

void dataInputs(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;

	GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}

void dataOutputs(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;

	GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}

void writeCpld(Byte data, Byte address)
{
	writeAddress (address);
	dataOutputs();

	writePin (data & 1, B, 12)
	writePin (data & 2, B, 13)
	writePin (data & 4, B, 14)
	writePin (data & 8, B, 15)
	writePin (data & 16, D, 2)
	writePin (data & 32, D, 3)
	writePin (data & 64, G, 6)
	writePin (data & 128, G, 7)

	writePin (0, I, 8)
	writePin (0, G, 12)
	writePin (1, I, 8)
	writePin (1, G, 12)
}

Byte readCpld(Byte address)
{
	Byte data = 0;

	
	writeAddress(address);
	dataInputs();

	writePin (0, C, 12)
	writePin (0, G, 12)
	
	data |= readPin (1, B, 12)
	data |= readPin (2, B, 13)
	data |= readPin (4, B, 14)
	data |= readPin (8, B, 15)
	data |= readPin (16, D, 2)
	data |= readPin (32, D, 3)
	data |= readPin (64, G, 6)
	data |= readPin (128, G, 7)
	
	writePin (1, C, 12)
	writePin (1, G, 12)
	
	return data;
}

void initGpioMem(void)
{
	writePin (1, G, 12)
	writePin (1, I, 8)
	writePin (1, C, 12)
}

// SDRAM
void sdramInit(void)
{
  // must initialize SDRAM with a command and auto refresh value
	FMC_SDRAM_CommandTypeDef sdc;

	sdc.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	sdc.ModeRegisterDefinition = 0;
	// step 1
	sdc.AutoRefreshNumber = 2;
	sdc.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
	HAL_SDRAM_SendCommand(&hsdram1, &sdc, 100);
	// step 2
	sdc.CommandMode = FMC_SDRAM_CMD_PALL;
	HAL_SDRAM_SendCommand(&hsdram1, &sdc, 100);
	// step 3
	sdc.AutoRefreshNumber = 5;
	sdc.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	HAL_SDRAM_SendCommand(&hsdram1, &sdc, 100);
	// step 4
	sdc.AutoRefreshNumber = 2;
	sdc.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	sdc.ModeRegisterDefinition = 0x0231; // sdram command MRD field in SDCMR reg
	HAL_SDRAM_SendCommand(&hsdram1, &sdc, 100);
	// step 5
	HAL_SDRAM_ProgramRefreshRate(&hsdram1, 683);
}

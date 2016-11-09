// UART link for CLI  Robert Chapman III  Nov 8, 2016

#include "uartLink.h"
#include "byteq.h"
#include "localio.h"
#include "machines.h"

static serialLink_t serialUart;

void serialUartHandler(void)
{
	serialLink_t * link = &serialUart;
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);
	uint32_t isrflags = READ_REG(huart->Instance->SR);

	if ((isrflags & USART_SR_RXNE) != RESET)
		link->linkIn(link);
	
	if ((isrflags & USART_SR_TXE) != RESET) {
		if (link->linkEmpty(link))
			CLEAR_BIT(huart->Instance->CR1, USART_CR1_TXEIE);
		else
			link->linkOut(link);
	}
}

static void serialUartIn(serialLink_t *link)
{
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);
	
	keyin(huart->Instance->DR);
}

static void serialUartOut(serialLink_t *link)
{
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);

	huart->Instance->DR = pullbq(link->txq);
}

static bool serialEmpty(serialLink_t *link)
{
	return qbq(link->txq) == 0;
}

void serialMachine(void)
{
	UART_HandleTypeDef * huart = serialUart.port;

	if (!serialEmpty(&serialUart))
		/* Enable the UART Transmit data register empty Interrupt */
    	SET_BIT(huart->Instance->CR1, USART_CR1_TXEIE);
	activate(serialMachine);
}

void initSerialLink(void)
{
	serialLink_t * link = &serialUart;
	UART_HandleTypeDef * huart = serialUart.port;

	link->txq = eq;
	link->linkIn = serialUartIn;
	link->linkOut = serialUartOut;
	link->linkEmpty = serialEmpty;
	link->port = &UART_PORT;
	
	/* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
	SET_BIT(huart->Instance->CR3, USART_CR3_EIE);

	/* Enable the UART Parity Error and Data Register not empty Interrupts */
	SET_BIT(huart->Instance->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);

	activateOnce(serialMachine);
}

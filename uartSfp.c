// Simple network: one node one link  Robert Chapman III  Nov 9, 2016

#include "stats.h"
#include "sfpRxSm.h"
#include "sfpTxSm.h"
#include "devices.h"

sfpLink_t uartLink;

QUEUE(MAX_FRAMES, uartFrameq);
QUEUE(MAX_FRAMES, uartNpsq);
QUEUE(MAX_FRAMES, uartSpsq);

// uart interrupt call out
void uartLinkHandler(void)
{
	sfpLink_t * link = &uartLink;
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);

	sfpRxSm(link);
	
	if ((READ_REG(huart->Instance->SR) & USART_SR_TXE) != RESET) {
		if (link->sfpBytesToTx)
			transmitSfpByte(link);
		else {
			sfpTxSm(link);
			if (link->frameOut == NULL)
				CLEAR_BIT(huart->Instance->CR1, USART_CR1_TXEIE);
		}
	}
}

// queue drivers
void uartPut(Long c, sfpLink_t *link)
{
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);
	
	BytesOut(link);
	huart->Instance->DR = (Byte)c;
}

// Run the SFP protocol layer
void serviceSfpUart(void)
{
	sfpLink_t *link = &uartLink;
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);

	if (queryq(link->npsq) || testAckSend(link) || testSpsSend(link))
		/* Enable the UART Transmit data register empty Interrupt */
    	SET_BIT(huart->Instance->CR1, USART_CR1_TXEIE);
}

static bool checkUart(sfpLink_t *link)
{
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);
	Short sr = READ_REG(huart->Instance->SR);
	
	return (sr & USART_SR_RXNE) != RESET;
}

static Byte getUart(sfpLink_t * link)
{
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);
	
	BytesIn(link);
	return huart->Instance->DR;
}

static bool uartEmpty(struct sfpLink_t * link)
{
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);

	return (READ_REG(huart->Instance->SR) & USART_SR_TXE) != RESET;
}

void initSfpUart(void)
{
	sfpLink_t *link  = &uartLink;
	UART_HandleTypeDef * huart = &SFP_UART;
	
	// initialize UART link
	initLink(link, "UART Link");
	link->port = huart;
	link->sfpRx = checkUart;
	link->sfpGet = getUart;
	link->sfpTx = uartEmpty;
	link->sfpPut = uartPut;

	// initialize state machines
	initSfpRxSM(link, uartFrameq);
	initSfpTxSM(link, uartNpsq, uartSpsq);

	/* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
	SET_BIT(huart->Instance->CR3, USART_CR3_EIE);

	/* Enable the UART Parity Error and Data Register not empty Interrupts */
	SET_BIT(huart->Instance->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);
}

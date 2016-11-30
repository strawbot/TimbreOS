// Simple network: one node one link  Robert Chapman III  Nov 9, 2016

#include "node.h"
#include "stats.h"
#include "sfpRxSm.h"
#include "sfpTxSm.h"
#include "framepool.h"
#include "services.h"
#include "talkhandler.h"
#include "printers.h"
#include "stm32f4xx_hal.h"
#include "devices.h"
#include "who.h"
#include "stm32f4xx_hal.h"
#include "localio.h"
#include "machines.h"

#define UART_PORT huart3

extern UART_HandleTypeDef huart3;

sfpNode_t myNode;
sfpLink_t uartLink;

QUEUE(MAX_FRAMES, uartFrameq);
QUEUE(MAX_FRAMES, uartNpsq);
QUEUE(MAX_FRAMES, uartSpsq);

void initVersion(void);

// queue drivers
void uartPut(Long c, sfpLink_t *link)
{
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);
	
	BytesOut(link);
	huart->Instance->DR = (Byte)c;
}

// Run the SFP protocol layer
void sfpMachine(void)
{
	sfpLink_t *link = &uartLink;
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);

	if (queryq(link->npsq) || testAckSend(link) || testSpsSend(link))
		/* Enable the UART Transmit data register empty Interrupt */
    	SET_BIT(huart->Instance->CR1, USART_CR1_TXEIE);

	processFrames();
	sendeqSfp();
	activate(sfpMachine);
}

static bool checkUart(sfpLink_t *link)
{
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);
	
	return (READ_REG(huart->Instance->SR) & USART_SR_RXNE) != RESET;
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

void initSfp(void)
{
	sfpLink_t *link;
	UART_HandleTypeDef * huart;
	
	// initialize pool of frame buffers
    initFramePool();

	// initialize the node
    initNode(&myNode);
    setNode(&myNode);
    setWhoami(MAIN_CPU);
    setWhatami(0);
 
	// initialize UART link
	link = &uartLink;
	initLink(link, "UART Link");
	link->port = huart = &UART_PORT;
	link->sfpRx = checkUart;
	link->sfpGet = getUart;
	link->sfpTx = uartEmpty;
	link->sfpPut = uartPut;

	// initialize state machines
	initSfpRxSM(link, uartFrameq);
	initSfpTxSM(link, uartNpsq, uartSpsq);

	addLink(0, link); // attached links
	setRouteTo(DIRECT, link);
    setRouteTo(MAIN_HOST, link); // routes for other nodes
    
	// initialize services and stats
    initSfpStats();
	initServices();
    
    activateOnce(sfpMachine);
	
	/* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
	SET_BIT(huart->Instance->CR3, USART_CR3_EIE);

	/* Enable the UART Parity Error and Data Register not empty Interrupts */
	SET_BIT(huart->Instance->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);

	initVersion();
	initTalkHandler();
}

void uartLinkHandler(void)
{
	sfpLink_t * link = &uartLink;
	UART_HandleTypeDef * huart = (UART_HandleTypeDef *)(link->port);
	uint32_t isrflags = READ_REG(huart->Instance->SR);

	sfpRxSm(link);
	
	if ((isrflags & USART_SR_TXE) != RESET) {
		if (link->sfpBytesToTx)
			transmitSfpByte(link);
		else {
			sfpTxSm(link);
			if (link->frameOut == NULL)
				CLEAR_BIT(huart->Instance->CR1, USART_CR1_TXEIE);
		}
	}
}

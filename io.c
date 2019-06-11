// handle user io  Robert Chapman III  Mar 1 2017

#include "byteq.h"
#include "cli.h"
#include "printers.h"
#include "machines.h"

// cli io
byteq_t * inputq = keyq;
byteq_t * outputq = emitq;

void setInputq(byteq_t * q) {
	inputq = q;
}

void setOutputq(byteq_t * q) {
	outputq = q;
}

void output() {
//	if (qbq(outputq) && (USART_StatusGet(UART1) & USART_STATUS_TXBL))
//		USART_Tx(UART1, pullbq(outputq));
}

void input() {
//	if (USART_StatusGet(UART1) & USART_STATUS_RXDATAV)
//		pushbq(USART_RxDataGet(UART1), inputq);
}

// machinery
void outputMachine() {
	output();
	activate(outputMachine);
}

void inputMachine() {
	input();
    activate(inputMachine);
}

void cliMachine() {
    cli();
    activate(cliMachine);
}

void initIo() { // call to set up and run ios
	initMachines();
    resetCli();
    print("Timbre CLI");
    dotPrompt();
    activateOnce(inputMachine);
    activateOnce(outputMachine);
    activateOnce(cliMachine);
}

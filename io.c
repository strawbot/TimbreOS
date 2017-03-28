// handle user io  Robert Chapman III  Mar 1 2017

#include "byteq.h"
#include "cli.h"
#include "printers.h"
#include "machines.h"

﻿// platform specific io
void output() {
    if (qbq(emitq) && (USART_StatusGet(UART1) & USART_STATUS_TXBL))
        USART_Tx(UART1, pullbq(emitq));
}

void input() {
    if (USART_StatusGet(UART1) & USART_STATUS_RXDATAV)
        pushbq(USART_RxDataGet(UART1), keyq);
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
﻿	initMachines();
    resetCli();
    print("Timbre CLI");
    dotPrompt();
    activateOnce(inputMachine);
    activateOnce(outputMachine);
    activateOnce(cliMachine);
}

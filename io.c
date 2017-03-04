// handle user io  Robert Chapman III  Mar 1 2017

#include "byteq.h"
#include "cli.h"
#include "printers.h"
#include "machines.h"

// cli io
void output() {
    if (qbq(emitq) && (USART_StatusGet(UART1) & USART_STATUS_TXBL))
        USART_Tx(UART1, pullbq(emitq));
    activate(output);
}

void input() {
    if (USART_StatusGet(UART1) & USART_STATUS_RXDATAV)
        pushbq(USART_RxDataGet(UART1), keyq);
    activate(input);
}

void handleCli() {
    cli();
    activate(handleCli);
}

void hello() {
    print("hi");
}

void initIo() { // call to set up and run ios
    resetCli();
    activateOnce(input);
    activateOnce(output);
    activateOnce(handleCli);
}

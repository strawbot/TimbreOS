#include <QtTest>

#include "support.h"
#include "testprinters.h"
#include "../printers.c"

testPrinters::testPrinters(QObject *parent) : QObject(parent)
{

}

void testPrinters::init()
{
    zeroEmits();
    hpStore();
    rpStore();
    spStore();
    decimal();
}

void testPrinters::testPrint(void)
{

}

void testPrinters::testPrintHex(void)
{

}

void testPrinters::testPrintnHex(void)
{

}

void testPrinters::testPrintDec(void)
{

}

void testPrinters::testPrintDec0(void)
{

}

void testPrinters::testPrintBin(void)
{

}

void testPrinters::testPrintChar(void)
{

}

void testPrinters::testPrintHex2(void)
{

}

void testPrinters::testFlush(void)
{

}

void testPrinters::testPrintFloat(void)
{

}


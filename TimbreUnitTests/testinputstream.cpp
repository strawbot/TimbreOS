#include <QtTest>

#include "support.h"
#include "testinputstream.h"

testinputstream::testinputstream(QObject *parent) : QObject(parent)
{

}

void testinputstream::init()
{
    zeroEmits();
    zeroTib();
    spStore();
    emptyKeyq();
    hpStore();
    autoEchoOff();
    leftBracket();
}

void testinputstream::testCli()
{
    Byte * input = getTib();

    inKey(LFEED);
    cli();
    QVERIFY(getTib() == input);
    QVERIFY(numEmits() == 0);
    QVERIFY(depth() == 0);

    inString("a");
    collector();
    QVERIFY(getTib() == input+1);
    QVERIFY(numEmits() == 0);
    QVERIFY(depth() == 0);

    inKey(BSPACE);
    cli();
    QVERIFY(getTib() == input);
    QVERIFY(numEmits() == 0);
    QVERIFY(depth() == 0);

    inKey(CRETURN);
    cli();
    QVERIFY(getTib() == input);
    QVERIFY(depth() == 0);

    inString("constantname");
    inKey(0);
    collector();
    QVERIFY(getTib() == input);
    QVERIFY(ret() == 12345);
    QVERIFY(depth() == 0);

    zeroEmits();
    inKey(1);
    collector();
    QVERIFY(numEmits() == 0);

    autoEchoOn();
    inKey(1);
    collector();
    QVERIFY(getEmit() == BEEP);
}

void testinputstream::testQuote()
{
    Byte * input = getTib();

    inKey(QUOTE);
    cli();
    QVERIFY(getTib() == input+1);
    inKey(BSPACE);
    cli();
    QVERIFY(getTib() == input);

    inKey(QUOTE);
    inString("blah");
    inKey(QUOTE);
    inKey(0);

    collector();
    QVERIFY(depth() == 1);
    count();
    QVERIFY(ret() == 4);
    QVERIFY(0 == strcmp((char *)ret(), "blah"));
}

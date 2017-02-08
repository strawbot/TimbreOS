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

void testinputstream::testCollectkeys()
{
    Byte * input = getTib();

    inKey(LFEED);
    collectKeys();
    QVERIFY(getTib() == input);
    QVERIFY(numEmits() == 0);
    QVERIFY(depth() == 0);

    inString("a");
    collector();
    QVERIFY(getTib() == input+1);
    QVERIFY(numEmits() == 0);
    QVERIFY(depth() == 0);

    inKey(BSPACE);
    collectKeys();
    QVERIFY(getTib() == input);
    QVERIFY(numEmits() == 0);
    QVERIFY(depth() == 0);

    inKey(CRETURN);
    collectKeys();
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

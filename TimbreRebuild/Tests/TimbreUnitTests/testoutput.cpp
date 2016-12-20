#include <QtTest>

#include "support.h"
#include "testoutput.h"

testoutput::testoutput(QObject *parent) : QObject(parent)
{

}

void testoutput::init()
{
    zeroEmits();
    hpStore();
    rpStore();
    spStore();
    decimal();
}

// output stream
void testoutput::TestSafeEmit()
{
    QVERIFY(numEmits() == 0);
    safeEmit('a');
    QVERIFY(numEmits() == 1);
    QVERIFY(getEmit() == 'a');
    QVERIFY(numEmits() == 0);
    QVERIFY(emitOverflow == false);
    QVERIFY(outs() == 0);
}

void testoutput::TestEmitByte()
{
    emitByte('a');
    QVERIFY(numEmits() == 1);
    QVERIFY(getEmit() == 'a');
    QVERIFY(outs() == 1);

    emitByte(8);
    QVERIFY(getEmit() == 8);
    QVERIFY(outs() == 0);

    emitByte(127);
    QVERIFY(getEmit() == 7);
    QVERIFY(outs() == 0);

    emitByte('a');
    emitByte(13);
    QVERIFY(getEmit() == 'a');
    QVERIFY(getEmit() == 13);
    QVERIFY(outs() == 0);

    emitByte(10);
    emitByte(7);
    QVERIFY(getEmit() == 10);
    QVERIFY(getEmit() == 7);
    QVERIFY(outs() == 0);
}

void testoutput::TestEmitOp()
{
    lit(45);
    emitOp();
    QVERIFY(getEmit() == 45);
    QVERIFY(depth() == 0);
}

void testoutput::TestCursorReturn()
{
    cursorReturn();
    QVERIFY(getEmit() == 10);
    QVERIFY(getEmit() == 13);
}

void testoutput::TestMaybeCr()
{
    emitByte('a');
    maybeCr();
    maybeCr();
    QCOMPARE(numEmits(), (Cell)3);
}

void testoutput::TestCount()
{
    Byte array[] = {6};

    lit((Cell)array);
    count();
    QCOMPARE(ret(), (Cell)6);
    QCOMPARE(ret(), (Cell)array+1);
    QVERIFY(depth() == 0);
}

void testoutput::TestType()
{
    Byte array[] = {2,'a','b'};

    lit((Cell)array);
    count();
    type();
    QCOMPARE(numEmits(), (Cell)2);
    QVERIFY(getEmit() == 'a');
    QVERIFY(getEmit() == 'b');
    QVERIFY(depth() == 0);
}

void testoutput::TestSpaces()
{
    spaces(2);
    QCOMPARE(numEmits(), (Cell)2);
    QVERIFY(getEmit() == ' ');
    QVERIFY(getEmit() == ' ');
}

void testoutput::TestHex()
{
    QCOMPARE(getBase(), (Byte)10);
    hex();
    QCOMPARE(getBase(), (Byte)16);
}

void testoutput::TestDecimal()
{
    hex();
    decimal();
    QCOMPARE(getBase(), (Byte)10);
}

void testoutput::TestHold()
{
    lit(10);
    hold();
    QVERIFY(qbq(padq) == 1);
    QVERIFY(pullbq(padq) == 10);
    QVERIFY(depth() == 0);
}

void testoutput::TestStartNumberConversion()
{
    lit(10);
    hold();
    startNumberConversion();
    QVERIFY(qbq(padq) == 0);
}

void testoutput::TestConvertDigit()
{
    lit(9);
    convertDigit();
    QVERIFY(pullbq(padq) == '9');
    QVERIFY(ret() == 0);

    lit(10);
    convertDigit();
    QVERIFY(pullbq(padq) == '0');
    QVERIFY(ret() == 1);

    hex();
    lit(9);
    convertDigit();
    QVERIFY(pullbq(padq) == '9');
    QVERIFY(ret() == 0);

    lit(10);
    convertDigit();
    QVERIFY(pullbq(padq) == 'A');
    QVERIFY(ret() == 0);

    QVERIFY(qbq(padq) == 0);
    QVERIFY(depth() == 0);
}

void testoutput::TestConvertNumber()
{
    lit(12345);
    convertNumber();
    QVERIFY(popbq(padq) == '1');
    QVERIFY(popbq(padq) == '2');
    QVERIFY(popbq(padq) == '3');
    QVERIFY(popbq(padq) == '4');
    QVERIFY(popbq(padq) == '5');

    QVERIFY(ret() == 0);
    QVERIFY(qbq(padq) == 0);
    QVERIFY(depth() == 0);
}

void testoutput::TestSign()
{
    lit(-1);
    lit(0);
    sign();
    QVERIFY(popbq(padq) == '-');
    QVERIFY(ret() == 0);
    QVERIFY(qbq(padq) == 0);

    lit(0);
    lit(-1);
    sign();
    QVERIFY((Integer)ret() == -1);
    QVERIFY(qbq(padq) == 0);
    QVERIFY(depth() == 0);
}

void testoutput::TestEndNumberConversion()
{
    lit(123);
    endNumberConversion();
    QVERIFY(ret() == 0);
    QVERIFY(ret() == padBuffer());
    QVERIFY(qbq(padq) == 0);
    QVERIFY(depth() == 0);
}

void testoutput::TestDotr()
{
    lit(-1);
    lit(3);
    dotr();
    QVERIFY(getEmit() == ' ');
    QVERIFY(getEmit() == '-');
    QVERIFY(getEmit() == '1');
    QVERIFY(qbq(padq) == 0);
    QVERIFY(depth() == 0);
}

void testoutput::TestDot()
{
    lit(-1);
    dot();
    QVERIFY(getEmit() == '-');
    QVERIFY(getEmit() == '1');
    QVERIFY(getEmit() == ' ');
    QVERIFY(qbq(padq) == 0);
    QVERIFY(depth() == 0);

    hex();
    lit(-1);
    dot();
    while(qbq(padq))
        QVERIFY(getEmit() == 'F');
}


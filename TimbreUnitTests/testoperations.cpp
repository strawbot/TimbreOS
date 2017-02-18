#include <QtTest>

#include "testoperations.h"
#include "support.h"

testoperations::testoperations(QObject *parent) : QObject(parent)
{

}

void testoperations::init()
{
    rpStore();
    spStore();
}

void testoperations::TestAnd()
{
    lit(0x1234);
    lit(0x1234);
    andOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0x1234);
    lit(0x1234);
    lit(0);
    andOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0);
}

void testoperations::TestOr()
{
    lit(0x1234);
    lit(0);
    orOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0x1234);
    lit(0x1234);
    lit(~0x1234);
    orOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == (Long)(-1));
}

void testoperations::TestXor()
{
    lit(0x1234);
    lit(0);
    xorOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0x1234);
    lit(0x1234);
    lit(0x1234);
    xorOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0);
}

void testoperations::TestNot()
{
    lit(0x1234);
    notOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == (Long)~0x1234);
}

void testoperations::TestShift()
{
    lit(1);
    lit(1);
    shiftOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 2);
    lit(2);
    lit((Cell)-1);
    shiftOp();
    QVERIFY(ret() == 1);
}

void testoperations::TestPlus()
{
    lit(1);
    lit(1);
    shiftOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 2);
}

void testoperations::TestMinus()
{
    lit(1);
    lit(1);
    minusOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0);
}

void testoperations::TestNegate()
{
    lit(1);
    negateOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == (Cell)-1);
}

void testoperations::TestSlashmod()
{
    lit(13);
    lit(5);
    slashModOp();
    QVERIFY(depth() == 2);
    QVERIFY(ret() == 2);
    QVERIFY(ret() == 3);
}

void testoperations::TestSlash()
{
    lit(13);
    lit(5);
    slashOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 2);
}

void testoperations::TestMod()
{
    lit(13);
    lit(5);
    modOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 3);
}

void testoperations::TestStar()
{
    lit(3);
    lit(2);
    starOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 6);
}

void testoperations::TestAbs()
{
    lit((Cell)-1);
    absOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 1);
    lit(1);
    absOp();
    QVERIFY(ret() == 1);
}

void testoperations::TestMax()
{
    lit(1);
    lit(2);
    maxOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 2);
    lit(2);
    lit(1);
    maxOp();
    QVERIFY(ret() == 2);
 }

void testoperations::TestMin()
{
    lit(1);
    lit(2);
    minOp();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 1);
    lit(2);
    lit(1);
    minOp();
    QVERIFY(ret() == 1);
}

// compare
void testoperations::TestEquals()
{
    lit(1);
    lit(2);
    equals();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == false);
    lit(1);
    lit(1);
    equals();
    QVERIFY(ret() == true);
}

void testoperations::TestLessThan()
{
    lit(1);
    lit(2);
    lessThan();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == true);
    lit(2);
    lit(1);
    lessThan();
    QVERIFY(ret() == false);
}

void testoperations::TestGreaterThan()
{
    lit(1);
    lit(2);
    greaterThan();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == false);
    lit(2);
    lit(1);
    greaterThan();
    QVERIFY(ret() == true);
}


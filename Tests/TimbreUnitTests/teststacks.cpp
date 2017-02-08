#include <QtTest>

#include "support.h"
#include "teststacks.h"

testStacks::testStacks(QObject *parent) : QObject(parent)
{

}

void testStacks::init()
{
    spStore();
    rpStore();
}

// data stack
void testStacks::TestLitRet()
{
    QVERIFY(depth() == 0);
    lit(0x1234);
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0x1234);
    QVERIFY(depth() == 0);
}

void testStacks::TestDup()
{
    lit(0x1234);
    QVERIFY(depth() == 1);
    dup();
    QVERIFY(depth() == 2);
    QVERIFY(ret() == 0x1234);
    QVERIFY(ret() == 0x1234);
}

void testStacks::TestSwap()
{
    lit(0x1234);
    lit(0x4321);
    swap();
    QVERIFY(ret() == 0x1234);
    QVERIFY(ret() == 0x4321);
}

void testStacks::TestOver()
{
    lit(0x1234);
    lit(0x4321);
    over();
    QVERIFY(ret() == 0x1234);
    QVERIFY(ret() == 0x4321);
    QVERIFY(ret() == 0x1234);
    QVERIFY(depth() == 0);
}

void testStacks::TestQdup()
{
    lit(0x1234);
    questionDup();
    QVERIFY(depth() == 2);
    QVERIFY(ret() == 0x1234);
    QVERIFY(ret() == 0x1234);
    lit(0);
    questionDup();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0);
}

// return stack
void testStacks::TestTor()
{
    QVERIFY(rdepth() == 0);
    lit(0x1234);
    tor();
    QVERIFY(depth() == 0);
    QVERIFY(rdepth() == 1);
}

void testStacks::TestRat()
{
    lit(0x1234);
    tor();
    rat();
    QVERIFY(rdepth() == 1);
    QVERIFY(ret() == 0x1234);
    QVERIFY(depth() == 0);
}

void testStacks::TestRfrom()
{
    lit(0x1234);
    tor();
    rfrom();
    QVERIFY(rdepth() == 0);
    QVERIFY(ret() == 0x1234);
}

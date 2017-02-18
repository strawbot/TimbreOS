#include <QtTest>

#include "support.h"
#include "testdefines.h"

testDefines::testDefines(QObject *parent) : QObject(parent)
{

}

void testDefines::init()
{
    resetCli();
    zeroEmits();
}

void testDefines::testColon()
{
    inputString("test 0x1234");
    colon();
    interpret();
    semiColon();
    inputString("nothing 0x134");
    colon();
    interpret();
    semiColon();
    inputString("test");
    interpret();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0x1234);
}

void testDefines::testConstant()
{
    lit(0x4321);
    inputString("test");
    constant();
    inputString("test");
    interpret();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0x4321);
}

void testDefines::testVariable()
{
    lit(0x54321);
    inputString("test");
    variable();
    inputString("test");
    interpret();
    QVERIFY(depth() == 1);
    fetch();
    QVERIFY(ret() == 0x54321);
}

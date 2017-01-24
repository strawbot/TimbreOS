#include <QtTest>

#include "support.h"
#include "testcompiler.h"

bool executed;

void exec()
{
    executed = true;
}

tcbody func = {.ii = exec};

testcompiler::testcompiler(QObject *parent) : QObject(parent)
{

}

void testcompiler::init()
{
    executed = false;
    leftBracket();
    hpStore();
    rpStore();
    spStore();
}

void testcompiler::testRightbracket()
{
    QVERIFY(getCompile() == 0);
    righBracket();
    QVERIFY(getCompile() != 0);
}

void testcompiler::testLeftbracket()
{
    righBracket();
    QVERIFY(getCompile() != 0);
    leftBracket();
    QVERIFY(getCompile() == 0);
}

void testcompiler::testCompileit()
{
    compileIt((tcbody *)1);
    QVERIFY(*(Cell *)testMemory == 1);
}

void testcompiler::testExecuteit()
{
    QVERIFY(executed == false);
    executeIt((tcbody *)&func);
    QVERIFY(executed == true);
    QVERIFY(getTick() == (Cell)&func);
}

void testcompiler::testExecute()
{
    lit((Cell)&func);
    execute();
    QVERIFY(executed == true);
    QVERIFY(depth() == 0);
}

void testcompiler::testLii()
{
    Cell c[] = {(Cell)colonii, getLii(), 0x1234, 0};

    executeIt((tcbody *)c);
    QVERIFY(ret() == 0x1234);
}

void testcompiler::testVii()
{
    Cell c[] = {(Cell)vii, 0x1234};

    executeIt((tcbody *)c);
    QVERIFY(ret() == (Cell)&c[1]);
}

void testcompiler::testCii()
{
    Cell c[] = {(Cell)cii, 0x1234};

    executeIt((tcbody *)c);
    QVERIFY(ret() == 0x1234);
}

void testcompiler::testColonii()
{
    Cell a[] = {(Cell)cii, 0x4321};
    Cell b[] = {(Cell)cii, 0x1234};
    Cell c[] = {(Cell)colonii, (Cell)a, 0};
    Cell d[] = {(Cell)colonii, (Cell)c, 0};
    Cell e[] = {(Cell)colonii, (Cell)d, (Cell)b, 0};

    executeIt((tcbody *)e);
    QVERIFY(ret() == 0x1234);
    QVERIFY(ret() == 0x4321);
}

void testcompiler::testLiteral()
{
    literal(123);
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 123);
    righBracket();
    lit((Cell)colonii);
    comma();
    literal(0xabcd);
    lit(0);
    comma();
    executeIt((tcbody *)testMemory);
    QVERIFY(ret() == 0xabcd);
}

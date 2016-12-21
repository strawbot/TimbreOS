#include <QtTest>

#include "support.h"
#include "testcompiler.h"

bool executed;

void exec()
{
    executed = true;
}

struct { vector tick; }func = {exec};

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
    compileIt(1);
    QVERIFY(*(Cell *)testMemory == 1);
}

void testcompiler::testExecuteit()
{
    QVERIFY(executed == false);
    executeIt(&func.tick);
    QVERIFY(executed == true);
    QVERIFY(getTick() == (Cell)&func.tick);
}

void testcompiler::testExecute()
{
    lit((Cell)&func.tick);
    execute();
    QVERIFY(executed == true);
    QVERIFY(depth() == 0);
}

void testcompiler::testLii()
{
    Cell c[] = {(Cell)colonii, getLii(), 0x1234, 0};

    executeIt((thread)c);
    QVERIFY(ret() == 0x1234);
}

void testcompiler::testVii()
{
    Cell c[] = {(Cell)vii, 0x1234};

    executeIt((thread)c);
    QVERIFY(ret() == (Cell)&c[1]);
}

void testcompiler::testCii()
{
    Cell c[] = {(Cell)cii, 0x1234};

    executeIt((thread)c);
    QVERIFY(ret() == 0x1234);
}

void testcompiler::testColonii()
{
    Cell a[] = {(Cell)cii, 0x4321};
    Cell b[] = {(Cell)cii, 0x1234};
    Cell c[] = {(Cell)colonii, (Cell)a, 0};
    Cell d[] = {(Cell)colonii, (Cell)b, (Cell)c, 0};

    executeIt((thread)d);
    QVERIFY(ret() == 0x4321);
    QVERIFY(ret() == 0x1234);
}

void testcompiler::testLiteral()
{
    righBracket();
    lit((Cell)colonii);
    comma();
    literal(0xabcd);
    lit(0);
    comma();
    executeIt((thread)testMemory);
    QVERIFY(ret() == 0xabcd);
}

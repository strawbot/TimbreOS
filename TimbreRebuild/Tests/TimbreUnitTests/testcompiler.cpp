#include <QtTest>

#include "support.h"
#include "testcompiler.h"

bool executed;

void exec()
{
    executed = true;
}

tcbody func = {.ii = exec};
Cell a[] = {(Cell)cii, 0x4321};
Cell b[] = {(Cell)cii, 0x1234};
Cell one[] = {(Cell)cii, 1};
Cell two[] = {(Cell)cii, 2};

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
    executeIt((tcbody *)b);
    QVERIFY(ret() == 0x1234);
}

void testcompiler::testColonii()
{
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

void testcompiler::testBranch()
{
    here();
    lit((Cell)colonii); // ] :ii ahead 0x1234 endif 0x1234 exit [
    comma();
    compileAhead();
    compileIt((tcbody *)a);
    compileEndif();
    compileIt((tcbody *)b);
    compileExit();
    execute();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 0x1234);
}

Headless(dup);
Headless(minusOp);
Headless(rat);

void testcompiler::testZerobranch()
{ // ] :ii 2 begin dup while dup 1 - repeat exit [
    here();
    lit((Cell)colonii);
    comma();
    compileIt((tcbody *)two);
    compileBegin();
    compileIt(&_dup);
    compileWhile();
    compileIt(&_dup);
    compileIt((tcbody *)one);
    compileIt(&_minusOp);
    compileRepeat();
    compileExit();
    execute();
    QVERIFY(depth() == 3);
    QVERIFY(ret() == 0);
    QVERIFY(ret() == 1);
    QVERIFY(ret() == 2);
}

void testcompiler::testMinusbranch()
{ // ] :ii 2 for r next exit [
    here();
    lit((Cell)colonii);
    comma();
    compileIt((tcbody *)two);
    compileFor();
    compileIt(&_rat);
    compileNext();
    compileExit();
    execute();
    QVERIFY(depth() == 2);
    QVERIFY(ret() == 0);
    QVERIFY(ret() == 1);
}

void testcompiler::testUntil()
{ // ] :ii 1 begin 1 - dup until exit [
    here();
    lit((Cell)colonii);
    comma();
    compileIt((tcbody *)one);
    compileBegin();
    compileIt((tcbody *)one);
    compileIt(&_minusOp);
    compileIt(&_dup);
    compileUntil();
    compileExit();
    execute();
    QVERIFY(depth() == 1);
    QVERIFY((Integer)ret() == -1);
}

void testcompiler::testElse()
{// ] :ii if  1 else 2 endif exit [
    here();
    lit((Cell)colonii);
    comma();
    compileIf();
    compileIt((tcbody *)one);
    compileElse();
    compileIt((tcbody *)two);
    compileEndif();
    compileExit();
    tor();
    lit(1);
    rat();
    execute();
    lit(0);
    rfrom();
    execute();
    QVERIFY(depth() == 2);
    QVERIFY((Integer)ret() == 2);
    QVERIFY((Integer)ret() == 1);
}

#include <QtTest>

#include "support.h"
#include "testparsing.h"

testparsing::testparsing(QObject *parent) : QObject(parent)
{

}

void testparsing::init()
{
    Byte * p;

    zeroTib();
    p = getTib();
    *p++ = ' ';
    *p++ = ' ';
    *p++ = 'a';
    *p++ = 0;
    zeroTib();
    hpStore();
}

void testparsing::testskip()
{
    Byte * q = getTib();

    skip(0);
    QVERIFY(getTib() - q == 0);
    skip(' ');
    QVERIFY(getTib() - q == 2);
    skip('a');
    QVERIFY(getTib() - q == 3);
    skip(0);
    QVERIFY(getTib() - q == 3);
}

void testparsing::testparse()
{
    parse('a');
    QVERIFY(testMemory[0] == 2);
    QVERIFY(testMemory[1] == ' ');
    QVERIFY(testMemory[2] == ' ');
    QVERIFY(testMemory[3] == 0);
    parse(' ');
    QVERIFY(testMemory[0] == 0);

    Byte * q = getTib();
    parse(0);
    QVERIFY(testMemory[0] == 0);
    QVERIFY(q == getTib());
}

void testparsing::testparseword()
{
    const char * string = (const char *)parseWord(' ');

    QVERIFY(strcmp(string, "a") == 0);
}

void testparsing::testcomment()
{
    strcpy((char *)getTib(), "( this is a comment)z");
    comment();
    QVERIFY(*getTib() == 'z');
    comment();
    QVERIFY(*getTib() == 0);
}

#include <QtTest>

#include "support.h"
#include "testmemory.h"

testmemory::testmemory(QObject *parent) : QObject(parent)
{

}

void testmemory::init()
{
    hpStore();
    rpStore();
    spStore();
}

// memory
void testmemory::TestHere()
{
    here();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Cell)testMemory);
}

void testmemory::TestAllot()
{
    allot(1);
    here();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Cell)testMemory+1);
 }

void testmemory::TestCComma()
{
    lit(0x12);
    cComma();
    here();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Cell)testMemory+sizeof(Byte));
    QCOMPARE(*(Cell *)testMemory, (Long)0x12);
}

void testmemory::TestAlign()
{
    Cell m;

    m = align((Cell)testMemory + 1);
    QCOMPARE(m, (Cell)testMemory+sizeof(Cell));
    m = align(m);
    QCOMPARE(m, (Cell)testMemory+sizeof(Cell));
}

void testmemory::TestComma()
{
    lit(0x1234);
    comma();
    here();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Cell)testMemory+sizeof(Cell));
    QCOMPARE(*(Cell *)testMemory, (Long)0x1234);
}

void testmemory::TestFetch()
{
    here();
    lit(0x1234);
    comma();
    fetch();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Long)0x1234);
}

void testmemory::TestStore()
{
    lit(0x4321);
    here();
    store();
    here();
    fetch();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Long)0x4321);
}

void testmemory::TestLongFetch()
{
    here();
    lit(0x12345678);
    comma();
    longFetch();
    QVERIFY(depth() == 2);
    QCOMPARE(ret(), (Long)0x1234);
    QCOMPARE(ret(), (Long)0x5678);
}

void testmemory::TestLongStore()
{
    lit(0x5678);
    lit(0x1234);
    here();
    longStore();
    here();
    longFetch();
    QVERIFY(depth() == 2);
    QCOMPARE(ret(), (Long)0x1234);
    QCOMPARE(ret(), (Long)0x5678);
}

void testmemory::TestShortFetch()
{
    *(Short *)testMemory = 0x1234;
    here();
    shortFetch();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Long)0x1234);
}

void testmemory::TestShortStore()
{
    lit(0x4321);
    here();
    shortStore();
    here();
    shortFetch();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Long)0x4321);
}

void testmemory::TestByteFetch()
{
    testMemory[0] = 0x12;
    here();
    byteFetch();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Long)0x12);
}

void testmemory::TestByteStore()
{
    lit(0x43);
    here();
    byteStore();
    here();
    byteFetch();
    QVERIFY(depth() == 1);
    QCOMPARE(ret(), (Long)0x43);
}

void testmemory::TestPlusBits()
{
    testMemory[0] = 0;
    lit(0x45);
    here();
    plusBits();
    QVERIFY(depth() == 0);
    QCOMPARE(testMemory[0], (Byte)0x45);
}

void testmemory::TestMinusBits()
{
    testMemory[0] = 0xFF;
    lit(0x45);
    here();
    minusBits();
    QVERIFY(depth() == 0);
    QCOMPARE(testMemory[0], (Byte)(0x45^0xFF));
}

void testmemory::TestByteMove()
{
    testMemory[0] = 0xab;
    testMemory[1] = 0;
    here();
    dup();
    lit(1);
    plusOp();
    lit(1);
    byteMove();
    QVERIFY(depth() == 0);
    QCOMPARE(testMemory[0], testMemory[1]);
}

void testmemory::TestByteFill()
{
    here();
    lit(2);
    lit(0xFE);
    byteFill();
    QVERIFY(depth() == 0);
    QCOMPARE(testMemory[0], (Byte)0xFE);
    QCOMPARE(testMemory[0], testMemory[1]);
 }

void testmemory::TestByteErase()
{
    here();
    lit(2);
    byteErase();
    QVERIFY(depth() == 0);
    QCOMPARE(testMemory[0], (Byte)0);
    QCOMPARE(testMemory[0], testMemory[1]);
}

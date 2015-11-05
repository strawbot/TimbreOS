#include <QtTest>

#include "testdictionary.h"

#include <set>

std::set<void *> memory;

void free (void * m)
{
    if (memory.end() != memory.find(m)) {
        memory.erase(m);
        delete (void **)m;
    }
    else
        printf("Freeing unmallocked memory!");
}

void *malloc(size_t size)
{
    void * m = new char [size];

    if (memory.end() == memory.find(m)) {
        memory.insert(m);
        return m;
    }
    printf("Allocated unfreed memory!");
    return m;
}

extern "C" {

#define error(msg)			print("Error: "#msg"\n")
#define note(msg)			print("Note: "#msg"\n")

#include "printers.c"
#include "dictionary.c"
}

dictionary_t testdict = {NULL, NULL, 0, 0};

testDictionary::testDictionary(QObject *parent) :
    QObject(parent)
{
}

const char firstChar = '!', lastChar = '~';

std::string randomString()
{
    Byte length = 1 + rand()%20; // length of string to be between 1 and 20 characters
    std::string random;
    random.resize(length);
    while(length--)
        random[length] = rand()%(lastChar-firstChar) + firstChar;
    return random;
}

void testDictionary::init()
{
    emptyDict(&testdict);
    initDict(&testdict, 1);
    srand(0);
    s = (char *)"string1";
    for (Short i=0; i<testdict.capacity; i++)
       if (testdict.table[i] != 0)
           print("Not Empty ");
//   initDict(&testdict, 1);
}
#if 0
void testDictionary::TestFree()
{
    Cell address;
    void * m = malloc(0);

    free(m);

    qfree(0);
    address = (Cell)allocate(0);
    if (address == 0)
        QFAIL("null allocation");
    qfree(address);
    address = (Cell)allocate(0);
    if (address == 0)
        QFAIL("null allocation");
    qfree(address);
}

void testDictionary::TestSizes()
{
    QCOMPARE((int)hashSize(1), 251);
    QCOMPARE((int)hashSize(251), 251);
    QCOMPARE((int)hashSize(252), 509);
    QCOMPARE((int)hashSize(510), 1021);
    QCOMPARE((int)hashSize(1022), 2039);
    QCOMPARE((int)hashSize(2040), 4093);
    QCOMPARE((int)hashSize(4094), 8191);
    QCOMPARE((int)hashSize(8192), 16381);
    QCOMPARE((int)hashSize(16382), 32749);
    QCOMPARE((int)hashSize(32750), 65521);
    QCOMPARE((int)hashSize(65535), 65521);
}

void testDictionary::TestInitDict()
{
    QVERIFY(testdict.adjunct == 0);
    QCOMPARE((int)testdict.capacity, 251);
    QCOMPARE((int)testdict.free, testdict.capacity-BUFFER);
    QVERIFY(testdict.table != 0);

    QVERIFY(testdict.table[0] == NULL);
    testdict.table[0] = zeroString;
    QVERIFY(testdict.table[0] != NULL);
    init();
    QVERIFY(testdict.table[0] == NULL);
    for (Short i=0; i<testdict.capacity; i++)
        QVERIFY(testdict.table[i] == NULL);
}

void testDictionary::TestEmptyDict()
{
    emptyDict(&testdict);
    QVERIFY(testdict.table == 0);
    QVERIFY(testdict.adjunct == 0);
    QVERIFY(0 != strcmp(s, zeroString));
}

void testDictionary::TestEntries()
{
    int n = testdict.free;

    plusEntry(&testdict);
    QCOMPARE(n-1, (int)testdict.free);
    minusEntry(&testdict);
    QCOMPARE(n, (int)testdict.free);
    for (int i=0; i<n; i++)
        plusEntry(&testdict);
    QCOMPARE(0, (int)testdict.free);
    plusEntry(&testdict);
    QCOMPARE(n-1, (int)testdict.free);
}

void testDictionary::TestCheckAdjunct()
{
    QVERIFY(testdict.adjunct == 0);
    checkAdjunct(&testdict);
    QVERIFY(testdict.adjunct != 0);
}

void testDictionary::TestUsed()
{
    QVERIFY(!used(NULL));
    QVERIFY(!used(zeroString));
    QVERIFY(used((char*)"zeroString"));
}

void testDictionary::TestSame()
{
    QVERIFY(same(zeroString, zeroString));
    QVERIFY(same((char*)"hello", (char*)"hello"));
    QVERIFY(!same((char*)"hello", zeroString));
}

void testDictionary::TestHash()
{
    QCOMPARE(false, hash((char *)"string1", &testdict) == hash((char *)"string2", &testdict));
    QCOMPARE(false, hash((char *)"hello", &testdict) == hash((char *)"world", &testdict));

    // test distribution: use half the capicity of random strings and double the content if already 1 otherwise set to 1
    // perfect distribution would yield a sum of half the capacity; more collisions to single location get exponentially worse
    checkAdjunct(&testdict);
    for (Byte i=0; i<testdict.capacity*100/100; i++) { // fill up 100% of the dictionary
        Cell * adj = dictAdjunct((char*)randomString().c_str(), &testdict);
        if (*adj)
            *adj *= 2;
        else
            *adj = 1;
    }
    Cell sum = 0;
    for (int i=0; i<testdict.capacity; i++)
        sum += testdict.adjunct[i];
    print("Over capacity by: "), printDec((float)sum*100/testdict.capacity - 100), print("%\n");
    QVERIFY(sum <  testdict.capacity + testdict.capacity*1/2);
}

void testDictionary::TestRehash()
{
    loc = hash(s, &testdict);
    QVERIFY(loc != rehash(s, loc, &testdict));
}

void testDictionary::TestLocate()
{
    loc = locate(s, &testdict);
    QVERIFY(*loc == NULL);
    *loc = s;
    QCOMPARE(loc, locate(s, &testdict));
    *loc = zeroString;
    QVERIFY(loc != locate(s, &testdict));
    *loc = (char*)"zeroString";
    QVERIFY(loc != locate(s, &testdict));
}

void testDictionary::TestBump()
{
    loc = hash(s, &testdict);
    QVERIFY(NULL == bump(s, loc));
    QVERIFY(s == bump(NULL, loc));
    checkAdjunct(&testdict);
    QVERIFY(0 == bumpAdjunct(24, loc, &testdict));
    QVERIFY(24 == bumpAdjunct(0, loc, &testdict));
    bumpAdjunct(24, loc, &testdict);
    deleteAdjunct(loc, &testdict);
    QVERIFY(0 == bumpAdjunct(0, loc, &testdict));
}

void testDictionary::TestInsert()
{
    Short n = testdict.free;

    for (Byte i=0; i<5; i++)
        dictInsert(s, &testdict);
    QVERIFY(n-5 == testdict.free);
    dictInsert((char*)randomString().c_str(), &testdict);
    QVERIFY(n-6 == testdict.free);
}
#endif

void testDictionary::TestAppend()
{
    Short n = testdict.free;

    for (Byte i=0; i<5; i++)
        dictAppend(s, &testdict);
    QVERIFY(n-5 == testdict.free);
    dictAppend((char*)randomString().c_str(), &testdict);
    QVERIFY(n-6 == testdict.free);
}
void testDictionary::TestDelete()
{
    Short n;

    n = testdict.free;
    dictDelete(s, &testdict);
    QVERIFY(n == testdict.free);
    dictAppend(s, &testdict);
    for (Short i=0; i<testdict.capacity; i++)
        if (testdict.table[i] != 0)
            print("Not Empty2 "), printHex((Cell)testdict.table[i]);
    dictDelete(s, &testdict);
    for (Short i=0; i<testdict.capacity; i++)
        if (testdict.table[i] != 0)
            print("Not Empty3 "), printHex((Cell)testdict.table[i]);
    QVERIFY(n == testdict.free);
    dictDelete(s, &testdict);
    for (Short i=0; i<testdict.capacity; i++)
        if (testdict.table[i] != 0)
            print("Not Empty4 "), printHex((Cell)testdict.table[i]);
    QVERIFY(n == testdict.free);
}
#if 0
void testDictionary::TestFind()
{
    char *other = (char *)std::string(s).c_str();

    QVERIFY(NULL == dictFind(s, &testdict));
    dictInsert(s, &testdict);       // s inserted
    QVERIFY(s == dictFind(s, &testdict));
    QVERIFY(s != other);
    QVERIFY(s == dictFind(other, &testdict));
    dictInsert(other, &testdict);   // other is first of finds
    QVERIFY(other == dictFind(s, &testdict));
    QVERIFY(s != dictFind(s, &testdict));
    dictDelete(s, &testdict);
    QVERIFY(other == dictFind(other, &testdict)); // finding zerostring here -> should find other; skip zero string
}
#endif

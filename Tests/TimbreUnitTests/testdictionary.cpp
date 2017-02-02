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

void breakpoint(void)
{
    printf("");
}


extern "C" {

#define errorMsg(msg)			print("Error: "#msg"\n")
#define note(msg)			print("Note: "#msg"\n")

#include "printers.c"
#include "dictionary.c"
}

dictionary_t testdict = {NULL, NULL, 0, 0, false};

testDictionary::testDictionary(QObject *parent) :
    QObject(parent)
{
}

const char firstChar = '!', lastChar = '~';

char * randomString()
{
    Byte length = 1 + rand()%20; // length of string to be between 1 and 20 characters
    std::string random;
    random.resize(length);
    while(length--)
        random[length] = rand()%(lastChar-firstChar) + firstChar;

    char * str = new char[strlen(random.c_str())+1];

    strcpy(str, random.c_str());
    return str;
}

void fillDictionary()
{
    while (testdict.free)
       dictInsert(randomString(), &testdict);

}

void testDictionary::init()
{
    freeDict(&testdict);
    initDict(&testdict, 1);
    srand(0);
    s = (char *)"string1";
    for (Short i=0; i<testdict.capacity; i++)
       if (testdict.table[i] != 0)
           print("Not Empty ");
//   initDict(&testdict, 1);
}

void testDictionary::TestFree()
{
    Cell address;
    void * m = malloc(0);

	(void)m;
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
    QCOMPARE((int)hashSize(251), 509);
    QCOMPARE((int)hashSize(252), 509);
    QCOMPARE((int)hashSize(510), 1021);
    QCOMPARE((int)hashSize(1022), 4093);
    QCOMPARE((int)hashSize(2040), 4093);
    QCOMPARE((int)hashSize(4094), 8191);
    QCOMPARE((int)hashSize(16381/2), 16381);
    QCOMPARE((int)hashSize(8192), 32749);
    QCOMPARE((int)hashSize(32749/2), 32749);
    QCOMPARE((int)hashSize(16382), 65521);
    QCOMPARE((int)hashSize(32750), 65521);
    QCOMPARE((int)hashSize(65535), 65521);
}

void testDictionary::TestInitDict()
{
    QVERIFY(testdict.adjunct != 0);
    QCOMPARE((int)testdict.capacity, 251);
    QCOMPARE((int)testdict.free, testdict.capacity/2);
    QVERIFY(testdict.table != 0);

    QVERIFY(testdict.table[0] == NULL);
    testdict.table[0] = zeroString;
    QVERIFY(testdict.table[0] != NULL);
    init();
    QVERIFY(testdict.table[0] == NULL);
    for (Short i=0; i<testdict.capacity; i++)
        QVERIFY(testdict.table[i] == NULL);
}

void testDictionary::TestfreeDict()
{
    freeDict(&testdict);
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
    QCOMPARE(testdict.capacity/2-1, (int)testdict.free);
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
    // test uniquity
    QVERIFY(hash((char *)"string1", &testdict) != hash((char *)"string2", &testdict));
    QVERIFY(hash((char *)"hello", &testdict) != hash((char *)"world", &testdict));

    // test boundaries
    for (Short i=0; i<testdict.capacity*10; i++)
        QVERIFY(testdict.capacity > hash(randomString(), &testdict));

    // test distribution: use percent of the capicity of random strings and double the content if already 1 otherwise set to 1
    // perfect distribution would yield a sum of part of the capacity; more collisions to single location get exponentially worse
    int percent = 50;
    for (int i=0; i<testdict.capacity*percent/100; i++) // fill up %age of the dictionary
        testdict.adjunct[locate(randomString(), &testdict)] += 1;

    Cell collisions = 0;
    for (int i=0; i<testdict.capacity; i++)
        if (testdict.adjunct[i])
            collisions += testdict.adjunct[i] - 1;
    printFloat((float)collisions*100/testdict.capacity, 1);
    print("% collisions when ");
    printDec(percent);
    print("% full\n");
    QVERIFY(collisions <  testdict.capacity);
}

void testDictionary::TestRehash()
{
    Short index = hash(s, &testdict);
    QVERIFY(index != rehash(s, index, &testdict));
    index = testdict.capacity;
    for (Short i=0; i<testdict.capacity*10; i++) {
        index = rehash(randomString(), testdict.capacity, &testdict);
        QVERIFY(index <= testdict.capacity);
    }
}

void testDictionary::TestLocate()
{
    Short index = locate(s, &testdict);
    QVERIFY(testdict.table[index] == NULL);
    testdict.table[index] = s;
    QCOMPARE(index, locate(s, &testdict));
    testdict.table[index] = zeroString;
    QVERIFY(index != locate(s, &testdict));
    testdict.table[index] = (char*)"zeroString";
    QVERIFY(index != locate(s, &testdict));
}

void testDictionary::TestInsert()
{
    Short n = testdict.free;

    for (Byte i=0; i<5; i++)
        dictInsert(s, &testdict);
    QVERIFY(n-5 == testdict.free);
    dictInsert(randomString(), &testdict);
    QVERIFY(n-6 == testdict.free);
}


void testDictionary::TestAppend()
{
    Short n = testdict.free;

    for (Byte i=0; i<5; i++)
        dictAppend(s, &testdict);
    QVERIFY(n-5 == testdict.free);
    dictAppend(randomString(), &testdict);
    QVERIFY(n-6 == testdict.free);
}

void testDictionary::TestDelete()
{
    Short n;

    n = testdict.free;
    dictDelete(s, &testdict);
    QVERIFY(n == testdict.free);
    dictAppend(s, &testdict);
    dictDelete(s, &testdict);
    QVERIFY(n == testdict.free);
    dictDelete(s, &testdict);
    QVERIFY(n == testdict.free);
}


void testDictionary::TestFind()
{
    char *clone = (char *)std::string(s).c_str();

    QVERIFY(s != clone); // different
    QVERIFY(0 == strcmp(s, clone)); // but the same
    QVERIFY(NULL == dictFind(s, &testdict));
    QVERIFY(NULL == dictFind(clone, &testdict));
    dictInsert(s, &testdict);       // s inserted
    QVERIFY(s == dictFind(s, &testdict));
    QVERIFY(s == dictFind(clone, &testdict));
    dictInsert(clone, &testdict);   // clone should be first of finds
    QVERIFY(clone == dictFind(s, &testdict));
    QVERIFY(s != dictFind(s, &testdict));
    dictDelete(s, &testdict); // delete first string match which should be clone
    QVERIFY(s == dictFind(clone, &testdict)); // finding zerostring here -> should find other; skip zero string
    dictDelete(clone, &testdict);
    QVERIFY(NULL == dictFind(s, &testdict));

    // check to see all get resolved properly
    emptyDict(&testdict);
    fillDictionary();
    for (Short i=0; i<testdict.capacity; i++) {
        char * string = testdict.table[i];

        if (used(string))
            QVERIFY(0 == strcmp(string, dictFind(string, &testdict)));
    }
}

void testDictionary::TestAdjunct()
{
    char *clone = (char *)std::string(s).c_str();
    char * string, * string2;

    QVERIFY(NULL == dictAdjunct(s, &testdict));
    QVERIFY(NULL == dictAdjunct(clone, &testdict));
    dictInsert(s, &testdict);
    QVERIFY(0 == *dictAdjunct(s, &testdict));
    *dictAdjunct(clone, &testdict) += 1;
    QVERIFY(1 == *dictAdjunct(s, &testdict));


    // test to see if adjunct gets moved with string inserts
    emptyDict(&testdict);
    int i = testdict.free;
    while (i--) {
       string = randomString();
       while (dictFind(string, &testdict)) // must be unique
           string = randomString();
       dictInsert(string, &testdict);
       *(char **)dictAdjunct(string, &testdict) = string;
    }

    // and again with appends
    emptyDict(&testdict);
    i = testdict.free;
    while (i--) {
       string = randomString();
       dictAppend(string, &testdict);
       *(char **)dictAdjunct(string, &testdict) = string;
       for (Short j=0; j<testdict.capacity; j++)
           if (testdict.table[j] != (char *)testdict.adjunct[j])
               breakpoint();
    }
    for (Short i=0; i<testdict.capacity; i++) { // check
        string = testdict.table[i];

        if (used(string)) {
            string2 = (char *)*dictAdjunct(string, &testdict);
            QVERIFY(0 != string2);
            QVERIFY(0 == strcmp(string, (char *)string2));
        }
    }
}

void testDictionary::TestUpsize()
{
    char * s1 = (char *)"string";
    char * s2 = (char *)"string";
    char * s3 = (char *)"other string";
    Short n = testdict.capacity;

    dictInsert(s1, &testdict);
    dictInsert(s2, &testdict);
    dictInsert(s3, &testdict);
    upsizeDict(&testdict);
    QVERIFY(n != testdict.capacity);
    QCOMPARE(dictFind(s2, &testdict), s2);
    QCOMPARE(dictFind(s3, &testdict), s3);
    dictDelete(s2, &testdict);
    QVERIFY(s1 == dictFind(s2, &testdict));

    // test upsize flag
    freeDict(&testdict);
    initDict(&testdict, 1);
    QVERIFY(n == testdict.capacity);
    fillDictionary();
    dictInsert(randomString(), &testdict);
    QVERIFY(n == testdict.capacity);

    // test auto overflow update
    freeDict(&testdict);
    initDict(&testdict, 1);
    fillDictionary();
    setUpsize(true, &testdict);
    dictInsert(randomString(), &testdict);
    QVERIFY(n != testdict.capacity);

    // test adjunct upsize and ordering
    dictInsert(s1, &testdict);
    dictInsert(s3, &testdict);
    *dictAdjunct(s1, &testdict) = 1;
    *dictAdjunct(s3, &testdict) = 2;
    upsizeDict(&testdict);
    QCOMPARE((int)*dictAdjunct(s1, &testdict), 1);
    QCOMPARE((int)*dictAdjunct(s3, &testdict), 2);

    // benchmark an upsize
    freeDict(&testdict);
    initDict(&testdict, HASH16);
    QVERIFY(HASH16 == testdict.capacity);
    while (testdict.free)
        dictInsert(randomString(), &testdict);
    QBENCHMARK_ONCE {upsizeDict(&testdict);}
    QVERIFY(n != testdict.capacity);
}

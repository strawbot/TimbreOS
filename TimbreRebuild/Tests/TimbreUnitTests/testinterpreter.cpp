#include <QtTest>

#include "support.h"
#include "testinterpreter.h"

testinterpreter::testinterpreter(QObject *parent) : QObject(parent)
{

}

#define NAME_BITS 0x80
#define IMMEDIATE_BITS (NAME_BITS | 0x40)
#define SMUDGE_BITS 0x20
#define HEADER_BITS (IMMEDIATE_BITS | SMUDGE_BITS)

typedef struct header32 {
    struct header32 * list;
    Byte name[32];
    vector tick;
} header32;

header32 name1 = {NULL, {4|NAME_BITS, 'N', 'a', 'm', 'e'}, NULL};
header32 name2 = {&name1, {6|NAME_BITS|SMUDGE_BITS, 'S', 'm', 'u', 'd', 'g', 'e'}, NULL};
header32 name3 = {&name2, {9|NAME_BITS|IMMEDIATE_BITS, 'I', 'm', 'm', 'e', 'd', 'i', 'a', 't', 'e'}, NULL};

Byte * string(const char * s)
{
    return (Byte *)s;
}

void testinterpreter::init()
{
    *(header32 **)getWordlist() = &name3;
    zeroEmits();
    hpStore();
    spStore();
    decimal();
}

void testinterpreter::testWordlist()
{
    QVERIFY(searchWordlist(string("")) == NULL);
    QVERIFY(searchWordlist(string("Name")) == &name1);
    QVERIFY(searchWordlist(string("Smudge")) == NULL);
    QVERIFY(searchWordlist(string("Immediate")) == &name3);
}

const char names[] = {"name1\000" "name2\000" "name3\000"};

extern "C" Short searchNames(Byte * name, const char * dictionary);
extern "C" Byte searchDictionaries(Byte * cstring, tcode * t);
extern "C" vector wordbodies[];
extern "C" void (*constantbodies[])();
extern "C" void (*immediatebodies[])();

void testinterpreter::testSearchnames()
{
    QVERIFY(searchNames(string("name0"), names) == 0);
    QVERIFY(searchNames(string("name2"), names) == 2);
}

void testinterpreter::testSearchdictionaries()
{
    tcode t;

    QVERIFY(searchDictionaries(string(""), &t) == 0);
    QVERIFY(searchDictionaries(string("wordname"), &t) == NAME_BITS);
    QVERIFY(t == (tcode)wordbodies);
    QVERIFY(searchDictionaries(string("immediatename"), &t) == IMMEDIATE_BITS);
    QVERIFY(t == (tcode)immediatebodies);
    QVERIFY(searchDictionaries(string("constantname"), &t) == NAME_BITS);
    QVERIFY(t == (tcode)constantbodies);
}

extern "C" tcode link2tick(header * link);

void testinterpreter::testLink2tick()
{
    struct {
        void * link;
        Byte name[5];
        vector tick;
        Cell value;
    } head = {NULL, {4|NAME_BITS,'n','a','m','e'}, cii, 0x123};

    QVERIFY(link2tick((header *)&head) == (tcode)&head.tick);
}

extern "C" Byte lookup(Byte * cstring, tcode * t);

void testinterpreter::testLookup()
{
    tcode t;

    QVERIFY(lookup(string(""), &t) == 0);
    QVERIFY(lookup(string("Name"), &t) == NAME_BITS); // can't check t since names are all 32 long
    QVERIFY(lookup(string("Smudge"), &t) == 0);
    QVERIFY(lookup(string("Immediate"), &t) == IMMEDIATE_BITS);
    QVERIFY(lookup(string(""), &t) == 0);
    QVERIFY(lookup(string("wordname"), &t) == NAME_BITS);
    QVERIFY(t == (tcode)wordbodies);
    QVERIFY(lookup(string("immediatename"), &t) == IMMEDIATE_BITS);
    QVERIFY(t == (tcode)immediatebodies);
    QVERIFY(lookup(string("constantname"), &t) == NAME_BITS);
    QVERIFY(t == (tcode)constantbodies);
}

extern "C" void msg(const char * m);
extern "C" void error(void);

void testinterpreter::testMsg()
{
    msg("Test");
    QVERIFY(getEmit() == 'T');
    QVERIFY(getEmit() == 'e');
    QVERIFY(getEmit() == 's');
    QVERIFY(getEmit() == 't');
    QVERIFY(numEmits() == 0);
}

void testinterpreter::testError()
{
    clearInterpretError();
    testMemory[0] = 1;
    testMemory[1] = 'X';
    error();
    QVERIFY(getEmit() == 'X');
    QVERIFY(getEmit() == '<');
    QVERIFY(numEmits() == 5);
    QVERIFY(getInterpretError() == 1);
}

extern "C" {
Byte checkBase(Byte * string);
bool toDigit(Byte *n);
Cell signDigits(Byte * string, bool sign);
Cell stringNumber(Byte * string);
}

void testinterpreter::testCheckBase()
{
    QVERIFY(checkBase(string("")) == 0);
    QVERIFY(checkBase(string("0")) == 0);
    QVERIFY(checkBase(string("00")) == 0);
    QVERIFY(checkBase(string("000")) == 0);
    QVERIFY(checkBase(string("0x0")) == 2);
    QVERIFY(getBase() == 16);
    QVERIFY(checkBase(string("0X0")) == 2);
    QVERIFY(getBase() == 16);
    QVERIFY(checkBase(string("0c0")) == 2);
    QVERIFY(getBase() == 8);
    QVERIFY(checkBase(string("0C0")) == 2);
    QVERIFY(getBase() == 8);
    QVERIFY(checkBase(string("0b0")) == 2);
    QVERIFY(getBase() == 2);
    QVERIFY(checkBase(string("0B0")) == 2);
    QVERIFY(getBase() == 2);
}

void testinterpreter::testToDigit()
{
    Byte c;

    c = '0';
    QVERIFY(toDigit(&c) == true);
    QVERIFY(c == 0);
    c = '9';
    QVERIFY(toDigit(&c) == true);
    QVERIFY(c == 9);
    c = 'A';
    QVERIFY(toDigit(&c) == false);
    c = '9'+1;
    QVERIFY(toDigit(&c) == false);
    hex();
    c = 'A';
    QVERIFY(toDigit(&c) == true);
    QVERIFY(c == 10);
    c = 'f';
    QVERIFY(toDigit(&c) == true);
    QVERIFY(c == 15);
    bin();
    c = '1';
    QVERIFY(toDigit(&c) == true);
    QVERIFY(c == 1);
    c = '2';
    QVERIFY(toDigit(&c) == false);
}

void testinterpreter::testSignDigits()
{
    Cell n;

    QVERIFY(signDigits(string(""), false) == 0);
    QVERIFY(signDigits(string(""), true) == 0);
    QVERIFY(signDigits(string("12345"), false) == 12345);
    QVERIFY((Integer)signDigits(string("12345"), true) == -12345);
    hex();
    QVERIFY(signDigits(string("1234abcd"), false) == 0x1234abcd);
    bin();
    QVERIFY(signDigits(string("0101010101"), false) == 0b0101010101);
    decimal();
    n = signDigits(string("1.3"), false);
    QVERIFY(fabs(*(float *)&n - 1.3) < .01);
    n = signDigits(string("1.3"), true);
    QVERIFY(fabs(*(float *)&n + 1.3) < .01);
}

void testinterpreter::testStringNumber()
{
    QVERIFY(stringNumber(string("12345")) == 12345);
    QVERIFY((Integer)stringNumber(string("-12345")) == -12345);
}

extern "C" {
void quit();
void interpret();
}

void testinterpreter::testQuit()
{
    quit();
    QVERIFY(getBase() == 10);
    QVERIFY(getInterpretError()== 0);
    QVERIFY(getCompile() == 0);
    QVERIFY(depth() == 0);
}

void testinterpreter::testInterpreter()
{
    strcpy((char *)getTib(), "1234");
    interpret();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 1234);
    zeroTib();
    strcpy((char *)getTib(), "constantname");
    interpret();
    QVERIFY(depth() == 1);
    QVERIFY(ret() == 12345);
    righBracket();
    zeroTib();
    strcpy((char *)getTib(), "constantname");
    interpret();
    QVERIFY(depth() == 0);
    here();
    QVERIFY(ret() == (Cell)&testMemory[align(1)]);
 }

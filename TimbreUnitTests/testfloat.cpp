#include <QtTest>

#include "support.h"
#include "testfloat.h"
#include "float.c"

extern "C" {
Cell signDigits(Byte * string, bool sign);
}

testFloat::testFloat(QObject *parent) : QObject(parent)
{

}

void testFloat::init()
{
    zeroEmits();
    hpStore();
    rpStore();
    spStore();
    decimal();
}

// test all cli calls in float.c

#include <QtTest>

#include "support.h"
#include "testtimeout.h"

NEW_TO(testto);

testTimeout::testTimeout(QObject *parent) : QObject(parent)
{

}

void testTimeout::init() {
    setTime(0);
}

void testTimeout::testCheck() {
    setTimeout(1, testto);
    QVERIFY(checkTimeout(testto) == false);
    oneMillisecondTick();
    QVERIFY(checkTimeout(testto) == true);
    oneMillisecondTick();
    QVERIFY(checkTimeout(testto) == true);
}

void testTimeout::testSet() {
    setTimeout(1, testto);
    QVERIFY(checkTimeout(testto) == false);
    oneMillisecondTick();
    QVERIFY(checkTimeout(testto) == true);
    setTimeout(1, testto);
    QVERIFY(checkTimeout(testto) == false);
}

void testTimeout::testSince() {
    setTimeout(1, testto);
    QVERIFY(sinceTimeout(testto) == -1);
    oneMillisecondTick();
    QVERIFY(sinceTimeout(testto) == 0);
    oneMillisecondTick();
    QVERIFY(sinceTimeout(testto) == 1);
}

void testTimeout::testStop() {
    setTimeout(1, testto);
    stopTimeout(testto);
    QVERIFY(checkTimeout(testto) == false);
    oneMillisecondTick();
    QVERIFY(checkTimeout(testto) == false);
}

void testTimeout::testStart() {
    setTimeout(1, testto);
    stopTimeout(testto);
    oneMillisecondTick();
    startTimeout(testto);
    QVERIFY(checkTimeout(testto) == true);
}

void testTimeout::testRepeat() {
    setTimeout(1, testto);
    oneMillisecondTick();
    repeatTimeout(testto);
    QVERIFY(checkTimeout(testto) == false);
    oneMillisecondTick();
    QVERIFY(checkTimeout(testto) == true);
    oneMillisecondTick();
    oneMillisecondTick();
    repeatTimeout(testto);
    QVERIFY(checkTimeout(testto) == true);
}

void testTimeout::testRestart() {
    setTimeout(1, testto);
    oneMillisecondTick();
    restartTimeout(testto);
    QVERIFY(checkTimeout(testto) == false);
    oneMillisecondTick();
    QVERIFY(checkTimeout(testto) == true);
    oneMillisecondTick();
    oneMillisecondTick();
    restartTimeout(testto);
    QVERIFY(checkTimeout(testto) == false);
    oneMillisecondTick();
    QVERIFY(checkTimeout(testto) == true);
}

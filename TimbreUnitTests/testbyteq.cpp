#include <QtTest>

#include "support.h"
#include "testbyteq.h"

#define tsize 1000

testbyteq::testbyteq(QObject *parent) : QObject(parent)
{

}

void testbyteq::init()
{

}

void testbyteq::testbq()
{
    Cell i, n = tsize;
    Byte qitem, pitem;
    BQUEUE(tsize, tq);

    qitem = pitem = 0;
    while(n) {
        zerobq(tq);

        for (i=0; i<n; i++)
            pushbq(qitem++, tq);

        pitem = bq(tq);
        for (i=0; i<tsize*2; i++) {
            Byte b;

            pushbq(qitem++, tq);
            b = pullbq(tq);
            QVERIFY(b == pitem);
            pitem++;
            QVERIFY(qbq(tq) == n);
        }
        n--;
    }
    QVERIFY(sizeof(tq) <= (3*sizeof(Cell) + tsize + 1 + sizeof(Cell)));
}

void testbyteq::testMinimumbq() {
    BQUEUE(0, minbq);
    QVERIFY(qbq(minbq) == 0);
    QVERIFY(fullbq(minbq) == true);
    QVERIFY(sizebq(minbq) == 0);
    QVERIFY(sizeof(minbq) == sizeof(byteq) + sizeof(Cell));
}

void testbyteq::testSizeofbq() {
    const int size = sizeof(Cell) - 1;
    BQUEUE(size, sbq);
    QVERIFY(sizebq(sbq) == size);
    QVERIFY(sizeof(sbq) == sizeof(byteq) + sizeof(Cell));
}

void testbyteq::testFullbq() {
    const int size = 31;
    BQUEUE(size, bq);
    while (!fullbq(bq))
        pushbq(0, bq);
    for (int i = 0; i<size; i++) {
        QVERIFY(fullbq(bq));
        QVERIFY(qbq(bq) != 0);
        pushbq(pullbq(bq), bq);
    }
}

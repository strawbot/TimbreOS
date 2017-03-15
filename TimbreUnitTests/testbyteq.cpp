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
}

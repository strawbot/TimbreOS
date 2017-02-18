#ifndef TESTMEMORY_H
#define TESTMEMORY_H

#include <QObject>

class testmemory : public QObject
{
    Q_OBJECT
public:
    explicit testmemory(QObject *parent = 0);

private slots:
    void init();
    void TestHere();
    void TestAllot();
    void TestCComma();
    void TestAlign();
    void TestComma();
    void TestFetch();
    void TestStore();
    void TestLongFetch();
    void TestLongStore();
    void TestShortFetch();
    void TestShortStore();
    void TestByteFetch();
    void TestByteStore();
    void TestPlusBits();
    void TestMinusBits();
    void TestByteMove();
    void TestByteFill();
    void TestByteErase();
};

#endif // TESTMEMORY_H

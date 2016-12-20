#ifndef TESTOUTPUT_H
#define TESTOUTPUT_H

#include <QObject>

class testoutput : public QObject
{
    Q_OBJECT
public:
    explicit testoutput(QObject *parent = 0);

signals:

private slots:
    void init();
    void TestSafeEmit();
    void TestEmitByte();
    void TestEmitOp();
    void TestCursorReturn();
    void TestMaybeCr();
    void TestCount();
    void TestType();
    void TestSpaces();
    void TestHex();
    void TestDecimal();
    void TestHold();
    void TestStartNumberConversion();
    void TestConvertDigit();
    void TestConvertNumber();
    void TestSign();
    void TestEndNumberConversion();
    void TestDotr();
    void TestDot();
};

#endif // TESTOUTPUT_H

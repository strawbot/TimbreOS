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
    void TestSafeEmitNest();
    void TestEmitByte();
    void TestEmitOp();
    void TestCursorReturn();
    void TestMaybeCr();
    void TestLength();
    void TestType();
    void TestSpaces();
    void TestBin();
    void TestOct();
    void TestDecimal();
    void TestHex();
    void TestHold();
    void TestStartNumberConversion();
    void TestConvertDigit();
    void TestConvertNumber();
    void TestSign();
    void TestEndNumberConversion();
    void TestDotr();
    void TestDot();
    void TestDotb();
    void TestDoth();
    void TestDotd();
    void TestDots();
    void TestSetPrompt();
    void TestdotPrompt();
    void TestWords();
};

#endif // TESTOUTPUT_H

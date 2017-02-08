#ifndef TESTSTACKS_H
#define TESTSTACKS_H

#include <QObject>

class testStacks : public QObject
{
    Q_OBJECT
public:
    explicit testStacks(QObject *parent = 0);

signals:

private slots:
    void init();
    void TestLitRet();
    void TestDup();
    void TestSwap();
    void TestOver();
    void TestQdup();
    void TestTor();
    void TestRat();
    void TestRfrom();
};

#endif // TESTSTACKS_H

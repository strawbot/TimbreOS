#ifndef TESTOPERATIONS_H
#define TESTOPERATIONS_H

#include <QObject>

class testoperations : public QObject
{
    Q_OBJECT
public:
    explicit testoperations(QObject *parent = 0);

signals:

private slots:
    void init();
    void TestAnd();
    void TestOr();
    void TestXor();
    void TestNot();
    void TestShift();
    void TestPlus();
    void TestMinus();
    void TestNegate();
    void TestSlashmod();
    void TestSlash();
    void TestMod();
    void TestStar();
    void TestAbs();
    void TestMax();
    void TestMin();
    void TestEquals();
    void TestLessThan();
    void TestGreaterThan();
};

#endif // TESTOPERATIONS_H

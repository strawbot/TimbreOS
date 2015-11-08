#ifndef TESTDICTIONARY_H
#define TESTDICTIONARY_H

#include <QObject>

class testDictionary : public QObject
{
    Q_OBJECT
public:
    explicit testDictionary(QObject *parent = 0);

signals:

private:
    char * s;
    char ** local;

private slots:
    void init();
    void TestFree();
    void TestSizes();
    void TestInitDict();
    void TestEmptyDict();
    void TestEntries();
    void TestCheckAdjunct();
    void TestUsed();
    void TestSame();
    void TestHash();
    void TestRehash();
    void TestLocate();
    void TestBump();
    void TestInsert();
    void TestAppend();
    void TestDelete();
    void TestFind();
    void TestAdjunct();
    void TestUpsize();
};

#endif // TESTDICTIONARY_H

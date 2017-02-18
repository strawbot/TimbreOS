#ifndef TESTINTERPRETER_H
#define TESTINTERPRETER_H

#include <QObject>

class testinterpreter : public QObject
{
    Q_OBJECT
public:
    explicit testinterpreter(QObject *parent = 0);

signals:

private slots:
    void init();
    void testWordlist();
    void testSearchnames();
    void testSearchdictionaries();
    void testLink2tick();
    void testLookup();
    void testMsg();
    void testError();
    void testCheckBase();
    void testToDigit();
    void testSignDigits();
    void testStringNumber();
    void testQuit();
    void testInterpreter();
};

#endif // TESTINTERPRETER_H

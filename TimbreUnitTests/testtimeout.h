#ifndef TESTTIMEOUT_H
#define TESTTIMEOUT_H

#include <QObject>

class testTimeout : public QObject
{
    Q_OBJECT
public:
    explicit testTimeout(QObject *parent = 0);

signals:

private slots:
    void init();
    void testCheck();
    void testSet();
    void testSince();
    void testStop();
    void testStart();
    void testRepeat();
    void testRestart();
};

#endif // TESTTIMEOUT_H

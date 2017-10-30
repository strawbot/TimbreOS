#ifndef TESTBYTEQ_H
#define TESTBYTEQ_H

#include <QObject>

class testbyteq : public QObject
{
    Q_OBJECT
public:
    explicit testbyteq(QObject *parent = 0);

signals:

private slots:
    void init();
    void testbq();
    void testMinimumbq();
    void testSizeofbq();
    void testFullbq();
};

#endif // TESTBYTEQ_H

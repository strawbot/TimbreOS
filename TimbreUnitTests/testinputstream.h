#ifndef TESTINPUTSTREAM_H
#define TESTINPUTSTREAM_H

#include <QObject>

class testinputstream : public QObject
{
    Q_OBJECT
public:
    explicit testinputstream(QObject *parent = 0);

signals:

private slots:
    void init();
    void testCli();
    void testQuote();
};

#endif // TESTINPUTSTREAM_H

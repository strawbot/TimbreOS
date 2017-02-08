#ifndef TESTPARSING_H
#define TESTPARSING_H

#include <QObject>

class testparsing : public QObject
{
    Q_OBJECT
public:
    explicit testparsing(QObject *parent = 0);

signals:

private slots:
    void init();
    void testskip();
    void testparse();
    void testparseword();
    void testcomment();
};

#endif // TESTPARSING_H

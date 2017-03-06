#ifndef TESTFLOAT_H
#define TESTFLOAT_H

#include <QObject>

class testFloat : public QObject
{
    Q_OBJECT
public:
    explicit testFloat(QObject *parent = 0);

private slots:
    void init();
};

#endif // TESTFLOAT_H

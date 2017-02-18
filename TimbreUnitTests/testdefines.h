#ifndef TESTDEFINES_H
#define TESTDEFINES_H

#include <QObject>

class testDefines : public QObject
{
    Q_OBJECT
public:
    explicit testDefines(QObject *parent = 0);

private slots:
    void init();
    void testColon();
    void testConstant();
    void testVariable();
};

#endif // TESTDEFINES_H

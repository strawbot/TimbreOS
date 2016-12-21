#ifndef TESTCOMPILER_H
#define TESTCOMPILER_H

#include <QObject>

class testcompiler : public QObject
{
    Q_OBJECT
public:
    explicit testcompiler(QObject *parent = 0);

signals:

private slots:
    void init();
    void testLeftbracket();
    void testRightbracket();
    void testCompileit();
    void testExecuteit();
    void testExecute();
    void testLii();
    void testVii();
    void testCii();
    void testColonii();
    void testLiteral();
};

#endif // TESTCOMPILER_H

#ifndef TESTPRINTERS_H
#define TESTPRINTERS_H

#include <QObject>

class testPrinters : public QObject
{
    Q_OBJECT
public:
    explicit testPrinters(QObject *parent = 0);

signals:

private slots:
    void init();
    void testDotnb(void);
    void testPrint(void);
    void testPrintHex(void);
    void testPrintnHex(void);
    void testPrintDec(void);
    void testPrintDec0(void);
    void testPrintBin(void);
    void testPrintChar(void);
    void testPrintHex2(void);
    void testFlush(void);
    void testPrintFloat(void);
    void testPdump(void);
    void testDump(void);
};

#endif // TESTPRINTERS_H

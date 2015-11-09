#include <QString>
#include <QtTest>

#include "testdictionary.h"

// Note: This is equivalent to QTEST_APPLESS_MAIN for multiple test classes.
int main(int argc, char** argv)
{
    int status = 0;

    {
        testDictionary tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    if (status != 0)
        qDebug() << "Errors occured during testing." << status ;

    return status;
}

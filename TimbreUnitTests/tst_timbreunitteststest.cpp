#include <QString>
#include <QtTest>

extern "C" {
#include "support.cpp"
}

#include "testbyteq.h"
#include "testdictionary.h"
#include "teststacks.h"
#include "testoperations.h"
#include "testmemory.h"
#include "testoutput.h"
#include "testprinters.h"
#include "testcompiler.h"
#include "testparsing.h"
#include "testinterpreter.h"
#include "testfloat.h"
#include "testinputstream.h"
#include "testdefines.h"
#include "testtimeout.h"

// Note: This is equivalent to QTEST_APPLESS_MAIN for multiple test classes.
int main(int argc, char** argv)
{
    int status = 0;

    {
        testbyteq tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testDictionary tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testStacks tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testoperations tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testmemory tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testoutput tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testPrinters tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testcompiler tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testparsing tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testinterpreter tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testFloat tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testinputstream tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testDefines tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        testTimeout tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    if (status != 0)
        qDebug() << "Errors occured during testing." << status ;

    return status;
}

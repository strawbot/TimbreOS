#include <QString>
#include <QtTest>

extern "C" {
#include "support.cpp"
}

#include "testdictionary.h"
#include "teststacks.h"
#include "testoperations.h"
#include "testmemory.h"
#include "testoutput.h"
#include "testcompiler.h"
#include "testparsing.h"
#include "testinterpreter.h"
#include "testinputstream.h"

// Note: This is equivalent to QTEST_APPLESS_MAIN for multiple test classes.
int main(int argc, char** argv)
{
    int status = 0;

//    {
//        testDictionary tc;
//        status |= QTest::qExec(&tc, argc, argv);
//    }

//    {
//        testStacks tc;
//        status |= QTest::qExec(&tc, argc, argv);
//    }

//    {
//        testoperations tc;
//        status |= QTest::qExec(&tc, argc, argv);
//    }

//    {
//        testmemory tc;
//        status |= QTest::qExec(&tc, argc, argv);
//    }

//    {
//        testoutput tc;
//        status |= QTest::qExec(&tc, argc, argv);
//    }

    {
        testcompiler tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

//    {
//        testparsing tc;
//        status |= QTest::qExec(&tc, argc, argv);
//    }

//    {
//        testinterpreter tc;
//        status |= QTest::qExec(&tc, argc, argv);
//    }

//    {
//        testinputstream tc;
//        status |= QTest::qExec(&tc, argc, argv);
//    }

    if (status != 0)
        qDebug() << "Errors occured during testing." << status ;

    return status;
}

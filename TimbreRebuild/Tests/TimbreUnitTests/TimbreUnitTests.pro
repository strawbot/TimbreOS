#-------------------------------------------------
#
# Project created by QtCreator 2015-10-27T17:10:13
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = tst_timbreunitteststest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += tst_timbreunitteststest.cpp \
    testdictionary.cpp \
    teststacks.cpp \
    testoperations.cpp \
    support.cpp \
    testmemory.cpp \
    testoutput.cpp \
    testcompiler.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += \
    testdictionary.h \
    teststacks.h \
    testoperations.h \
    support.h \
    testmemory.h \
    testoutput.h \
    testcompiler.h

INCLUDEPATH += ../../

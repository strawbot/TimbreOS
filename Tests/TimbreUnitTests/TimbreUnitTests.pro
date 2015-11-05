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
    testdictionary.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += \
    testdictionary.h

INCLUDEPATH += ../../

TEMPLATE = app
DEPENDPATH += ./ ../
INCLUDEPATH += ./ ../
DESTDIR = ./
CONFIG += qtestlib debug

DEFINES = TMPDIR=\\\"$${TMPDIR}\\\"

TARGET = logcrawler_tests
HEADERS += testlogdata.h testlogfiltereddata.h logdata.h logfiltereddata.h logdataworkerthread.h\
    abstractlogdata.h logfiltereddataworkerthread.h
SOURCES += testlogdata.cpp testlogfiltereddata.cpp abstractlogdata.cpp logdata.cpp main.cpp\
    logfiltereddata.cpp logdataworkerthread.cpp logfiltereddataworkerthread.cpp

coverage:QMAKE_CXXFLAGS += -g -fprofile-arcs -ftest-coverage -O0
coverage:QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage

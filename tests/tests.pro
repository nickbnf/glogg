TEMPLATE = app
DEPENDPATH += ./ ../
INCLUDEPATH += ./ ../
DESTDIR = ./
CONFIG += qtestlib debug

DEFINES = TMPDIR=\\\"$${TMPDIR}\\\"

TARGET = logcrawler_tests
HEADERS += testlogdata.h logdata.h logfiltereddata.h logdataworkerthread.h abstractlogdata.h
SOURCES += testlogdata.cpp abstractlogdata.cpp logdata.cpp main.cpp logfiltereddata.cpp\
    logdataworkerthread.cpp

coverage:QMAKE_CXXFLAGS += -g -fprofile-arcs -ftest-coverage -O0
coverage:QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage

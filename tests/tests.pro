TEMPLATE = app
DEPENDPATH += ./ ../
INCLUDEPATH += ./ ../
DESTDIR = ./
CONFIG += qtestlib debug

QMAKE_CXXFLAGS += -O2 -fpermissive

isEmpty(TMPDIR) {
    DEFINES = TMPDIR=\\\"/tmp\\\"
}
else {
    DEFINES = TMPDIR=\\\"$${TMPDIR}\\\"
}

mac {
  CONFIG -= app_bundle
}

TARGET = logcrawler_tests
HEADERS += testlogdata.h testlogfiltereddata.h logdata.h logfiltereddata.h logdataworkerthread.h\
    abstractlogdata.h logfiltereddataworkerthread.h filewatcher.h marks.h
SOURCES += testlogdata.cpp testlogfiltereddata.cpp abstractlogdata.cpp logdata.cpp main.cpp\
    logfiltereddata.cpp logdataworkerthread.cpp logfiltereddataworkerthread.cpp filewatcher.cpp\
    marks.cpp

coverage:QMAKE_CXXFLAGS += -g -fprofile-arcs -ftest-coverage -O0
coverage:QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage

prof:QMAKE_CXXFLAGS += -pg
prof:QMAKE_LFLAGS   += -pg

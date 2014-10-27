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
HEADERS += testlogdata.h testlogfiltereddata.h\
    ../src/data/logdata.h ../src/data/logfiltereddata.h ../src/data/logdataworkerthread.h\
    ../src/data/abstractlogdata.h ../src/data/logfiltereddataworkerthread.h\
    ../src/platformfilewatcher.h ../src/marks.h
SOURCES += testlogdata.cpp testlogfiltereddata.cpp \
    ../src/data/abstractlogdata.cpp ../src/data/logdata.cpp ../src/main.cpp\
    ../src/data/logfiltereddata.cpp ../src/data/logdataworkerthread.cpp\
    ../src/data/logfiltereddataworkerthread.cpp\
    ../src/filewatcher.cpp ../src/marks.cpp

coverage:QMAKE_CXXFLAGS += -g -fprofile-arcs -ftest-coverage -O0
coverage:QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage

prof:QMAKE_CXXFLAGS += -pg
prof:QMAKE_LFLAGS   += -pg

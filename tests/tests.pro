TEMPLATE = app
DEPENDPATH += ./ ../
INCLUDEPATH += ./ ../
DESTDIR = ./
CONFIG += qtestlib debug

TARGET = logcrawler_tests
HEADERS += logdata_test.h logdata.h logfiltereddata.h
SOURCES += logdata_test.cpp abstractlogdata.cpp logdata.cpp main.cpp logfiltereddata.cpp

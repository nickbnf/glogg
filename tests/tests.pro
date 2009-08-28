TEMPLATE = app
DEPENDPATH += ./ ../
INCLUDEPATH += ./ ../
DESTDIR = ./
CONFIG += qtestlib

TARGET = logcrawler_tests
HEADERS += logdata_test.h
SOURCES += logdata_test.cpp logdata.cpp main.cpp

# -------------------------------------------------
# LogCrawler
# -------------------------------------------------
TARGET = logcrawler
TEMPLATE = app
win32:debug:CONFIG += console

# Input
SOURCES += main.cpp \
    mainwindow.cpp \
    crawlerwidget.cpp \
    abstractlogdata.cpp \
    logdata.cpp \
    logfiltereddata.cpp \
    abstractlogview.cpp \
    logmainview.cpp \
    filteredview.cpp \
    optionsdialog.cpp \
    configuration.cpp \
    filtersdialog.cpp \
    filterset.cpp

HEADERS += version.h \
    mainwindow.h \
    crawlerwidget.h \
    logmainview.h \
    log.h \
    filteredview.h \
    abstractlogdata.h \
    logdata.h \
    logfiltereddata.h \
    abstractlogview.h \
    optionsdialog.h \
    configuration.h \
    filtersdialog.h \
    filterset.h

FORMS += filtersdialog.ui

debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
release:MOC_DIR = $${OUT_PWD}/.moc/release-shared

QMAKE_CXXFLAGS += -DLCRAWLER_DATE=\\\"`date \
    +'\"%F\"'`\\\"
QMAKE_CXXFLAGS += -DLCRAWLER_VERSION=\\\"`git \
    describe`\\\"
QMAKE_CXXFLAGS += -DLCRAWLER_COMMIT=\\\"`git \
    rev-parse --short \
    HEAD`\\\"

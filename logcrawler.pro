# -------------------------------------------------
# LogCrawler
# -------------------------------------------------
TARGET = logcrawler
TEMPLATE = app

win32:Debug:CONFIG += console

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
    filterset.cpp \
    savedsearches.cpp

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
    filterset.h \
    savedsearches.h

FORMS += filtersdialog.ui

RESOURCES = logcrawler.qrc

debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
release:MOC_DIR = $${OUT_PWD}/.moc/release-shared

Release:DEFINES += FILELOG_MAX_LEVEL=\"logERROR\"
Debug:DEFINES += FILELOG_MAX_LEVEL=\"logDEBUG\"

QMAKE_CXXFLAGS += -DLCRAWLER_DATE=\\\"`date \
    +'\"%F\"'`\\\"
QMAKE_CXXFLAGS += -DLCRAWLER_VERSION=\\\"`git \
    describe`\\\"
QMAKE_CXXFLAGS += -DLCRAWLER_COMMIT=\\\"`git \
    rev-parse --short \
    HEAD`\\\"

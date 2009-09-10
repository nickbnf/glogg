#-------------------------------------------------
# LogCrawler
#-------------------------------------------------

TARGET = logcrawler
TEMPLATE = app

CONFIG += console

# Input
SOURCES += main.cpp\
        mainwindow.cpp\
        crawlerwidget.cpp\
        logmainview.cpp\
        abstractlogdata.cpp\
        logdata.cpp\
        logfiltereddata.cpp\
        filteredview.cpp\
        abstractlogview.cpp\
        optionsdialog.cpp\
        configuration.cpp

HEADERS  += version.h\
        mainwindow.h\
        crawlerwidget.h\
        logmainview.h\
        logdata.h\
        logfiltereddata.h\
        log.h\
        filteredview.h\
        abstractlogview.h\
        optionsdialog.h\
        configuration.h

QMAKE_CXXFLAGS += -DLCRAWLER_DATE=\\\"`date +'\"%a_%b_%d,_%Y\"'`\\\"
QMAKE_CXXFLAGS += -DLCRAWLER_VERSION=\\\"`git describe`\\\"

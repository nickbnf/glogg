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
        abstractlogdata.cpp\
        logdata.cpp\
        logfiltereddata.cpp\
        abstractlogview.cpp\
        logmainview.cpp\
        filteredview.cpp\
        optionsdialog.cpp\
        configuration.cpp

HEADERS  += version.h\
        mainwindow.h\
        crawlerwidget.h\
        logmainview.h\
        log.h\
        filteredview.h\
        abstractlogdata.h\
        logdata.h\
        logfiltereddata.h\
        abstractlogview.h\
        optionsdialog.h\
        configuration.h

QMAKE_CXXFLAGS += -DLCRAWLER_DATE=\\\"`date +'\"%F\"'`\\\"
QMAKE_CXXFLAGS += -DLCRAWLER_VERSION=\\\"`git describe`\\\"
QMAKE_CXXFLAGS += -DLCRAWLER_COMMIT=\\\"`git rev-parse HEAD`\\\"

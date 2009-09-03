#-------------------------------------------------
# LogCrawler
#-------------------------------------------------

TARGET = logcrawler
TEMPLATE = app

# Input
SOURCES += main.cpp\
        mainwindow.cpp\
        crawlerwidget.cpp\
        logmainview.cpp\
        logdata.cpp\
        logfiltereddata.cpp\
        filteredview.cpp

HEADERS  += version.h\
        mainwindow.h\
        crawlerwidget.h\
        logmainview.h\
        logdata.h\
        logfiltereddata.h\
        log.h\
        filteredview.h

QMAKE_CXXFLAGS += -DLCRAWLER_DATE=\\\"`date +'\"%a_%b_%d,_%Y\"'`\\\"
QMAKE_CXXFLAGS += -DLCRAWLER_VERSION=\\\"`git describe`\\\"

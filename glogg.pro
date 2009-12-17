# -------------------------------------------------
# glogg
# -------------------------------------------------
TARGET = glogg
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
    savedsearches.cpp \
    infoline.cpp \
    logdataworkerthread.cpp \
    logfiltereddataworkerthread.cpp

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
    savedsearches.h \
    infoline.h \
    logdataworkerthread.h \
    logfiltereddataworkerthread.h

greaterThan(QT_VERSION, "4.4.0") {
    FORMS += filtersdialog.ui
}
else {
    message(Using old FiltersDialog)
    FORMS += filtersdialog_old.ui
}

# For Windows icon
RC_FILE = glogg.rc
RESOURCES = glogg.qrc

# Install (for unix)
icon16.path  = $$PREFIX/share/icons/hicolor/16x16/apps
icon16.files = images/hicolor/16x16/glogg.png

icon32.path  = $$PREFIX/share/icons/hicolor/32x32/apps
icon32.files = images/hicolor/32x32/glogg.png

doc.path  = $$PREFIX/share/doc/glogg
doc.files = README.textile COPYING

desktop.path = $$PREFIX/share/applications
desktop.path = glogg.desktop

target.path = $$PREFIX/bin
INSTALLS = target icon16 icon32 doc desktop

# Build directories
debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
release:MOC_DIR = $${OUT_PWD}/.moc/release-shared

Release:DEFINES += FILELOG_MAX_LEVEL=\"logERROR\"
Debug:DEFINES += FILELOG_MAX_LEVEL=\"logDEBUG\"

# Official builds can be generated with `qmake VERSION="1.2.3"'
isEmpty(VERSION) {
    QMAKE_CXXFLAGS += -DGLOGG_DATE=\\\"`date +'\"%F\"'`\\\"
    QMAKE_CXXFLAGS += -DGLOGG_VERSION=\\\"`git describe`\\\"
    QMAKE_CXXFLAGS += -DGLOGG_COMMIT=\\\"`git rev-parse --short HEAD`\\\"
}
else {
    QMAKE_CXXFLAGS += -DGLOGG_VERSION=\\\"$$VERSION\\\"
}

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
target.path = $$PREFIX/bin
INSTALLS = target

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

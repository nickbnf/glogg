# -------------------------------------------------
# glogg
# -------------------------------------------------

# Debug builds: qmake CONFIG+=debug
# Release builds: qmake

TARGET = klogg
TEMPLATE = app

QT += network core widgets concurrent

# Build directories
CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
}

OBJECTS_DIR = $${OUT_PWD}/.obj/$${DESTDIR}-shared
MOC_DIR = $${OUT_PWD}/.moc/$${DESTDIR}-shared
UI_DIR = $${OUT_PWD}/.ui/$${DESTDIR}-shared

CONFIG += c++14

!win32-msvc* {
    # Extra compiler arguments
    QMAKE_CXXFLAGS += -Wall -Wextra

    release:QMAKE_CXXFLAGS += -O2 -Werror
    #release:QMAKE_CXXFLAGS += -O0 -g #for release-debugging
}

GPROF {
    QMAKE_CXXFLAGS += -pg
    QMAKE_LFLAGS   += -pg
}

# Performance measurement
perf {
    QMAKE_CXXFLAGS += -DGLOGG_PERF_MEASURE_FPS
}

# Input
SOURCES += \
    src/main.cpp \
    src/session.cpp \
    src/data/abstractlogdata.cpp \
    src/data/logdata.cpp \
    src/data/logfiltereddata.cpp \
    src/data/logfiltereddataworkerthread.cpp \
    src/data/logdataworkerthread.cpp \
    src/data/compressedlinestorage.cpp \
    src/mainwindow.cpp \
    src/crawlerwidget.cpp \
    src/abstractlogview.cpp \
    src/logmainview.cpp \
    src/filteredview.cpp \
    src/optionsdialog.cpp \
    src/persistentinfo.cpp \
    src/configuration.cpp \
    src/filtersdialog.cpp \
    src/filterset.cpp \
    src/savedsearches.cpp \
    src/infoline.cpp \
    src/menuactiontooltipbehavior.cpp \
    src/selection.cpp \
    src/quickfind.cpp \
    src/quickfindpattern.cpp \
    src/quickfindwidget.cpp \
    src/sessioninfo.cpp \
    src/recentfiles.cpp \
    src/overview.cpp \
    src/overviewwidget.cpp \
    src/marks.cpp \
    src/quickfindmux.cpp \
    src/signalmux.cpp \
    src/tabbedcrawlerwidget.cpp \
    src/viewtools.cpp

INCLUDEPATH += src/

HEADERS += \
    src/data/abstractlogdata.h \
    src/data/logdata.h \
    src/data/logfiltereddata.h \
    src/data/logfiltereddataworkerthread.h \
    src/data/logdataworkerthread.h \
    src/data/threadprivatestore.h \
    src/data/compressedlinestorage.h \
    src/data/linepositionarray.h \
    src/mainwindow.h \
    src/session.h \
    src/viewinterface.h \
    src/crawlerwidget.h \
    src/logmainview.h \
    src/log.h \
    src/filteredview.h \
    src/abstractlogview.h \
    src/optionsdialog.h \
    src/persistentinfo.h \
    src/configuration.h \
    src/filtersdialog.h \
    src/filterset.h \
    src/savedsearches.h \
    src/infoline.h \
    src/filewatcher.h \
    src/selection.h \
    src/quickfind.h \
    src/quickfindpattern.h \
    src/quickfindwidget.h \
    src/sessioninfo.h \
    src/persistable.h \
    src/recentfiles.h \
    src/menuactiontooltipbehavior.h \
    src/overview.h \
    src/overviewwidget.h \
    src/marks.h \
    src/qfnotifications.h \
    src/quickfindmux.h \
    src/signalmux.h \
    src/tabbedcrawlerwidget.h \
    src/loadingstatus.h \
    src/externalcom.h \
    src/viewtools.h \
    src/data/atomicflag.h


FORMS += src/optionsdialog.ui
FORMS += src/filtersdialog.ui

RESOURCES = glogg.qrc

# Official builds can be generated with `qmake VERSION="1.2.3"'
include(glogg_version.pri)

win32 {
    include(glogg_win32.pri)
}

macx {
    include(glogg_mac.pri)
}

include(glogg_doc.pri)

# Install (for unix)
include(glogg_install.pri)


isEmpty(LOG_LEVEL) {
    CONFIG(debug, debug|release) {
        DEFINES += FILELOG_MAX_LEVEL=\"logDEBUG4\"
    } else {
        DEFINES += FILELOG_MAX_LEVEL=\"logDEBUG\"
    }
}
else {
    message("Using specified log level: $$LOG_LEVEL")
    DEFINES += FILELOG_MAX_LEVEL=\"$$LOG_LEVEL\"
}

# Optional features (e.g. CONFIG+=no-dbus)
system(pkg-config --exists QtDBus):!no-dbus:!win32 {
    message("Support for D-BUS will be included")
    QT += dbus
    QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_DBUS
    SOURCES += src/dbusexternalcom.cpp
    HEADERS += src/dbusexternalcom.h
}
else {
    message("Support for D-BUS will NOT be included")
    message("Support for cross-platform IPC will be included")
    QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_SOCKETIPC
    SOURCES += src/socketexternalcom.cpp
    HEADERS += src/socketexternalcom.h
}

# Version checking
version_checker {
    message("Version checker will be included")
    QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_VERSION_CHECKING
    SOURCES += src/versionchecker.cpp
    HEADERS += src/versionchecker.h
}
else {
    message("Version checker will NOT be included")
}

# File watching (e.g. CONFIG+=no-native-filewatch)
no-native-filewatch {
    message("File watching using Qt")
    QMAKE_CXXFLAGS += -DGLOGG_USES_QTFILEWATCHER
    SOURCES += src/qtfilewatcher.cpp
    HEADERS += src/qtfilewatcher.h
}
else {
    linux-g++ || linux-g++-64 {
        message("File watching using inotify")
        QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_INOTIFY
        SOURCES += src/platformfilewatcher.cpp src/inotifywatchtowerdriver.cpp src/watchtower.cpp src/watchtowerlist.cpp
        HEADERS += src/platformfilewatcher.h src/inotifywatchtowerdriver.h src/watchtower.h src/watchtowerlist.h
    }
}


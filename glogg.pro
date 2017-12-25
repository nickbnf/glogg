# -------------------------------------------------
# glogg
# -------------------------------------------------

# Debug builds: qmake CONFIG+=debug
# Release builds: qmake

TARGET = glogg
TEMPLATE = app

QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += core widgets

win32:Debug:CONFIG += console

# Necessary when cross-compiling:
win32:Release:QMAKE_LFLAGS += "-Wl,-subsystem,windows"

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
    src/viewtools.cpp \
    src/encodingspeculator.cpp \
    src/gloggapp.cpp \

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
    src/encodingspeculator.h \
    src/gloggapp.h \

isEmpty(BOOST_PATH) {
    message(Building using system dynamic Boost libraries)
    macx {
      # Path for brew installed libs
      INCLUDEPATH += /usr/local/include
      LIBS += -L/usr/local/lib -lboost_program_options-mt
    }
    else {
      LIBS += -lboost_program_options
    }
}
else {
    message(Building using static Boost libraries at $$BOOST_PATH)

    SOURCES += $$BOOST_PATH/libs/program_options/src/*.cpp \
        $$BOOST_PATH/libs/smart_ptr/src/*.cpp

    INCLUDEPATH += $$BOOST_PATH
}

FORMS += src/optionsdialog.ui
FORMS += src/filtersdialog.ui

macx {
    # Icon for Mac
    ICON = images/glogg.icns
}
else {
    # For Windows icon
    RC_ICONS = glogg48.ico
    QMAKE_TARGET_COMPANY = "Nicolas Bonnefon"
    QMAKE_TARGET_DESCRIPTION = "glogg - the fast, smart log explorer"
}

RESOURCES = glogg.qrc

# Build HTML documentation (if 'markdown' is available)
system(type markdown >/dev/null) {
    MARKDOWN += doc/documentation.markdown
}
else {
    message("markdown not found, HTML doc will not be generated")
}

doc_processor.name = markdown
doc_processor.input = MARKDOWN
doc_processor.output = doc/${QMAKE_FILE_BASE}.html
doc_processor.commands = markdown ${QMAKE_FILE_NAME} | \
    sed -f finish.sed >${QMAKE_FILE_OUT}

doc_processor.CONFIG += target_predeps
doc_processor.variable_out = doc.files

QMAKE_EXTRA_COMPILERS += doc_processor

# Install (for unix)
icon16.path  = $$PREFIX/share/icons/hicolor/16x16/apps
icon16.files = images/hicolor/16x16/glogg.png

icon32.path  = $$PREFIX/share/icons/hicolor/32x32/apps
icon32.files = images/hicolor/32x32/glogg.png

icon_svg.path  = $$PREFIX/share/icons/hicolor/scalable/apps
icon_svg.files = images/hicolor/scalable/glogg.svg

doc.path  = $$PREFIX/share/doc/glogg
doc.files += README COPYING

desktop.path = $$PREFIX/share/applications
desktop.files = glogg.desktop

target.path = $$PREFIX/bin
INSTALLS = target icon16 icon32 icon_svg doc desktop

# Build directories
CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
}

OBJECTS_DIR = $${OUT_PWD}/.obj/$${DESTDIR}-shared
MOC_DIR = $${OUT_PWD}/.moc/$${DESTDIR}-shared
UI_DIR = $${OUT_PWD}/.ui/$${DESTDIR}-shared

# Debug symbols even in release build
QMAKE_CXXFLAGS = -g

# Which compiler are we using
system( $${QMAKE_CXX} --version | grep -e " 4\\.[7-9]" ) || macx {
    message ( "g++ version 4.7 or newer, supports C++11" )
    CONFIG += C++11
}
else {
    CONFIG += C++0x
}

# Extra compiler arguments
# QMAKE_CXXFLAGS += -Weffc++
QMAKE_CXXFLAGS += -Wextra
C++0x:QMAKE_CXXFLAGS += -std=c++0x
C++11:QMAKE_CXXFLAGS += -std=c++11

GPROF {
    QMAKE_CXXFLAGS += -pg
    QMAKE_LFLAGS   += -pg
}

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

macx {
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_LFLAGS += -stdlib=libc++

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8

    QMAKE_INFO_PLIST = Info.plist
}

# Official builds can be generated with `qmake VERSION="1.2.3"'
isEmpty(VERSION):system(date >/dev/null) {
    system([ -f .tarball-version ]) {
        QMAKE_CXXFLAGS += -DGLOGG_VERSION=\\\"`cat .tarball-version`\\\"
    }
    else {
        QMAKE_CXXFLAGS += -DGLOGG_DATE=\\\"`date +'\"%F\"'`\\\"
        QMAKE_CXXFLAGS += -DGLOGG_VERSION=\\\"`git describe`\\\"
        QMAKE_CXXFLAGS += -DGLOGG_COMMIT=\\\"`git rev-parse --short HEAD`\\\"
    }
}
else {
    QMAKE_CXXFLAGS += -DGLOGG_VERSION=\\\"$$VERSION\\\"
}

# Optional features (e.g. CONFIG+=no-dbus)
system(pkg-config --exists QtDBus):!no-dbus {
    message("Support for D-BUS will be included")
    QT += dbus
    QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_DBUS
    SOURCES += src/dbusexternalcom.cpp
    HEADERS += src/dbusexternalcom.h
}
else {
    message("Support for D-BUS will NOT be included")
    win32 || macx {
        message("Support for cross-platform IPC will be included")
        QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_SOCKETIPC
        SOURCES += src/socketexternalcom.cpp
        HEADERS += src/socketexternalcom.h
    }
}

# Version checking
version_checker {
    message("Version checker will be included")
    QT += network
    QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_VERSION_CHECKING
    SOURCES += src/versionchecker.cpp
    HEADERS += src/versionchecker.h
}
else {
    message("Version checker will NOT be included")
}

# File watching
linux-g++ || linux-g++-64 {
    CONFIG += inotify
}

win32 {
    message("File watching using Windows")
    SOURCES += src/platformfilewatcher.cpp src/winwatchtowerdriver.cpp src/watchtower.cpp src/watchtowerlist.cpp
    HEADERS += src/platformfilewatcher.h src/winwatchtowerdriver.h src/watchtower.h src/watchtowerlist.h
    QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_POLLING
}
else {
    inotify {
        message("File watching using inotify")
        QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_INOTIFY
        SOURCES += src/platformfilewatcher.cpp src/inotifywatchtowerdriver.cpp src/watchtower.cpp src/watchtowerlist.cpp
        HEADERS += src/platformfilewatcher.h src/inotifywatchtowerdriver.h src/watchtower.h src/watchtowerlist.h
    }
    else {
        message("File watching using Qt")
        SOURCES += src/qtfilewatcher.cpp
        HEADERS += src/qtfilewatcher.h
    }
}

# Performance measurement
perf {
    QMAKE_CXXFLAGS += -DGLOGG_PERF_MEASURE_FPS
}

# -------------------------------------------------
# glogg
# -------------------------------------------------
TARGET = glogg
TEMPLATE = app

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

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
    src/filewatcher.cpp \
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
    src/signalmux.cpp

INCLUDEPATH += src/

HEADERS += \
    src/data/abstractlogdata.h \
    src/data/logdata.h \
    src/data/logfiltereddata.h \
    src/data/logfiltereddataworkerthread.h \
    src/data/logdataworkerthread.h \
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
    src/tabbedcrawlerwidget.h

isEmpty(BOOST_PATH) {
    message(Building using system dynamic Boost libraries)
    LIBS += -lboost_program_options
}
else {
    message(Building using static Boost libraries at $$BOOST_PATH)

    SOURCES += $$BOOST_PATH/libs/program_options/src/*.cpp \
        $$BOOST_PATH/libs/smart_ptr/src/*.cpp

    INCLUDEPATH += $$BOOST_PATH
}

FORMS += src/optionsdialog.ui
FORMS += src/filtersdialog.ui

# For Windows icon
RC_FILE = glogg.rc
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
debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
release:MOC_DIR = $${OUT_PWD}/.moc/release-shared
debug:UI_DIR = $${OUT_PWD}/.ui/debug-shared
release:UI_DIR = $${OUT_PWD}/.ui/release-shared

# Debug symbols in debug builds
# debug:QMAKE_CXXFLAGS += -g -O0

# Which compiler are we using
system( $${QMAKE_CXX} --version | grep -e " 4\.[7-9]" ) {
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
    Release:DEFINES += FILELOG_MAX_LEVEL=\"logERROR\"
    Debug:DEFINES += FILELOG_MAX_LEVEL=\"logDEBUG4\"
}
else {
    message("Using specified log level: $$LOG_LEVEL")
    DEFINES += FILELOG_MAX_LEVEL=\"$$LOG_LEVEL\"
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


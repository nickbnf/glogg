# -------------------------------------------------
# glogg
# -------------------------------------------------
TARGET = glogg
TEMPLATE = app

win32:Debug:CONFIG += console
# Necessary when cross-compiling:
win32:Release:QMAKE_LFLAGS += "-Wl,-subsystem,windows"

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
    persistentinfo.cpp \
    configuration.cpp \
    filtersdialog.cpp \
    filterset.cpp \
    savedsearches.cpp \
    infoline.cpp \
    logdataworkerthread.cpp \
    logfiltereddataworkerthread.cpp \
    filewatcher.cpp \
    selection.cpp \
    quickfind.cpp \
    quickfindpattern.cpp \
    quickfindwidget.cpp \
    sessioninfo.cpp \
    recentfiles.cpp \
    menuactiontooltipbehavior.cpp \
    overview.cpp \
    overviewwidget.cpp \
    marks.cpp

HEADERS += \
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
    persistentinfo.h \
    configuration.h \
    filtersdialog.h \
    filterset.h \
    savedsearches.h \
    infoline.h \
    logdataworkerthread.h \
    logfiltereddataworkerthread.h \
    filewatcher.h \
    selection.h \
    quickfind.h \
    quickfindpattern.h \
    quickfindwidget.h \
    sessioninfo.h \
    persistable.h \
    recentfiles.h \
    menuactiontooltipbehavior.h \
    overview.h \
    overviewwidget.h \
    marks.h

isEmpty(BOOST_PATH) {
    message(Building using system dynamic Boost libraries)
    LIBS += -lboost_program_options
}
else {
    message(Building using static Boost libraries at $$BOOST_PATH)

    SOURCES += $$BOOST_PATH/libs/program_options/src/*.cpp \
        $$BOOST_PATH/libs/detail/*.cpp \
        $$BOOST_PATH/libs/smart_ptr/src/*.cpp

    INCLUDEPATH += $$BOOST_PATH
}

FORMS += optionsdialog.ui

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

# Debug symbols in debug builds
debug:QMAKE_CXXFLAGS += -g

# Extra compiler arguments
# QMAKE_CXXFLAGS += -Weffc++

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


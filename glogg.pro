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
    logfiltereddataworkerthread.cpp \
    filewatcher.cpp

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
    logfiltereddataworkerthread.h \
    filewatcher.h

isEmpty(USE_NATIVE_BOOST) {
    message(Building using static Boost libraries included in the tarball)

    SOURCES += boost/libs/program_options/src/*.cpp \
        boost/libs/detail/*.cpp \
        boost/libs/smart_ptr/src/*.cpp

    INCLUDEPATH += ./boost
}
else {
    message(Building using native dynamic Boost libraries)
    LIBS += -lboost_program_options -L$HOME/lib/
}

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

doc.path  = $$PREFIX/share/doc/glogg
doc.files += README COPYING

desktop.path = $$PREFIX/share/applications
desktop.files = glogg.desktop

target.path = $$PREFIX/bin
INSTALLS = target icon16 icon32 doc desktop

# Build directories
debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
release:MOC_DIR = $${OUT_PWD}/.moc/release-shared

isEmpty(LOG_LEVEL) {
    Release:DEFINES += FILELOG_MAX_LEVEL=\"logERROR\"
    Debug:DEFINES += FILELOG_MAX_LEVEL=\"logDEBUG\"
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
    QMAKE_CXXFLAGS += -DGLOGG_VERSION=\\\"$(VERSION)\\\"
}


win32:Debug:CONFIG += console

# Necessary when cross-compiling:
win32:Release:QMAKE_LFLAGS += "-Wl,-subsystem,windows"

RC_ICONS = glogg48.ico
QMAKE_TARGET_COMPANY = "Anton Filimonov"
QMAKE_TARGET_DESCRIPTION = "Klogg log viewer"

!no-native-filewatch {
    message("File watching using Windows")
    SOURCES += src/platformfilewatcher.cpp src/winwatchtowerdriver.cpp src/watchtower.cpp src/watchtowerlist.cpp
    HEADERS += src/platformfilewatcher.h src/winwatchtowerdriver.h src/watchtower.h src/watchtowerlist.h
    QMAKE_CXXFLAGS += -DGLOGG_SUPPORTS_POLLING
}

win32-msvc* {
    LIBS += -luser32
}

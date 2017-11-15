isEmpty(VERSION):system(date >/dev/null) {
    system([ -f .tarball-version ]) {
        QMAKE_CXXFLAGS += -DGLOGG_VERSION=\\\"`cat .tarball-version`\\\"
    }
    else {
        QMAKE_CXXFLAGS += -DGLOGG_DATE=\\\"`date +'\"%F\"'`\\\"
        QMAKE_CXXFLAGS += -DGLOGG_VERSION=\\\"`git --git-dir=$$PWD/.git describe`\\\"
        QMAKE_CXXFLAGS += -DGLOGG_COMMIT=\\\"`git --git-dir=$$PWD/.git rev-parse --short HEAD`\\\"
    }
}
else {
    QMAKE_CXXFLAGS += -DGLOGG_VERSION=\\\"$$VERSION\\\"
}

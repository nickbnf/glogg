# Building glogg on Mac OS X

### prerequisites

1.  Install XCode from Apple's developers website.
2.  Install HomeBrew from: http://brew.sh
3.  Install boost and QT4 using homebrew:

        $ brew install boost
        $ brew install boost-build
        $ brew install qt

### compiling glogg

Clone the latest version from github:

    $ git clone https://github.com/nickbnf/glogg.git
    $ cd glogg

Run `qmake`, the output should be similar to the following:

    $ qmake
    Project MESSAGE: Building using system dynamic Boost libraries
    Project MESSAGE: g++ version 4.7 or newer, supports C++11
    Project MESSAGE: Support for D-BUS will NOT be included
    Project MESSAGE: Version checker will NOT be included
    Project MESSAGE: File watching using Qt

Run `make`, wait for the compliation to complete:

    $ make
    /usr/local/Cellar/qt/4.8.7/bin/uic src/optionsdialog.ui -o .ui/release-shared/ui_optionsdialog.h
    /usr/local/Cellar/qt/4.8.7/bin/uic src/filtersdialog.ui -o .ui/release-shared/ui_filtersdialog.h
    clang++ -c -g -Wextra -std=c++11 -stdlib=libc++ -DGLOGG_DATE=\"`date +"%F"`\" 
            -DGLOGG_VERSION=\"`git describe`\" -DGLOGG_COMMIT=\"`git rev-parse --short HEAD`\"
            -O2 -arch x86_64 -Wall -W -DFILELOG_MAX_LEVEL="logDEBUG" -DQT_NO_DEBUG -DQT_GUI_LIB
            -DQT_CORE_LIB -DQT_SHARED -I/usr/local/Cellar/qt/4.8.7/mkspecs/unsupported/macx-clang-libc++
            -I. -I/usr/local/Cellar/qt/4.8.7/lib/QtCore.framework/Versions/4/Headers
            -I/usr/local/Cellar/qt/4.8.7/lib/QtCore.framework/Versions/4/Headers
            -I/usr/local/Cellar/qt/4.8.7/lib/QtGui.framework/Versions/4/Headers
            -I/usr/local/Cellar/qt/4.8.7/lib/QtGui.framework/Versions/4/Headers
            -I/usr/local/Cellar/qt/4.8.7/include -Isrc -I/usr/local/include
            -I.moc/release-shared -I.ui/release-shared -F/usr/local/Cellar/qt/4.8.7/lib
            -o .obj/release-shared/main.o src/main.cpp
    [...]

The compiled program will be stored in the `release` sub-directory:

    $ ls -log ./release
    total 0
    drwxr-xr-x 3 102 Nov 27 15:05 glogg.app

To run glogg, use `open release/glogg.app/` or click on the program in *Finder*.

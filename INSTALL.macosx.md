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

### Preparing for distribution

Using the `macdeployqt` program from HomeBrew's `qt` package,
the glogg application can be bundled with the Qt framrworks and made into
a stand-alone DMG file:

    $ macdeployqt release/glogg.app -verbose=2 -dmg
    Log:
    Log: Deploying Qt frameworks found inside: ("release/glogg.app/Contents/MacOS/glogg")
    Log:  copied: "/usr/local/lib/libboost_program_options-mt.dylib"
    Log:  to "release/glogg.app/Contents/Frameworks//libboost_program_options-mt.dylib"
    Log:  copied: "/usr/local/lib/QtGui.framework/Versions/4/QtGui"
    Log:  to "release/glogg.app/Contents/Frameworks/QtGui.framework/Versions/4/QtGui"
    Log:  copied: "/usr/local/lib/QtGui.framework/Resources/qt_menu.nib/classes.nib"
    Log:  to "release/glogg.app/Contents/Frameworks/QtGui.framework/Resources/qt_menu.nib/classes.nib"
    Log:  copied: "/usr/local/lib/QtGui.framework/Resources/qt_menu.nib/info.nib"
    Log:  to "release/glogg.app/Contents/Frameworks/QtGui.framework/Resources/qt_menu.nib/info.nib"
    Log:  copied: "/usr/local/lib/QtGui.framework/Resources/qt_menu.nib/keyedobjects.nib"
    Log:  to "release/glogg.app/Contents/Frameworks/QtGui.framework/Resources/qt_menu.nib/keyedobjects.nib"
    Log:  copied: "/usr/local/lib/QtCore.framework/Versions/4/QtCore"
    Log:  to "release/glogg.app/Contents/Frameworks/QtCore.framework/Versions/4/QtCore"
    Log:
    Log: Deploying plugins from "/usr/local/plugins"
    Log: Created configuration file: "release/glogg.app/Contents/Resources/qt.conf"
    Log: This file sets the plugin search path to "release/glogg.app/Contents/PlugIns"
    Log:
    Log: Creating disk image (.dmg) for "release/glogg.app"

The compiled binary will use local dylibs/frameworks (with relative paths):

    $ otool -L glogg.app/Contents/MacOS/glogg
    glogg.app/Contents/MacOS/glogg:
        @executable_path/../Frameworks/libboost_program_options-mt.dylib (compatibility version 0.0.0, current version 0.0.0)
        @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui (compatibility version 4.8.0, current version 4.8.7)
        @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore (compatibility version 4.8.0, current version 4.8.7)
        /usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.0.0)
        /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1213.0.0)

The a `glogg.dmg` file will be created, and be ready for download/distribution:

    $ ls -log release
    total 6616
    drwxr-xr-x 3     102 Nov 28 00:58 glogg.app
    -rw-r--r-- 1 6768749 Nov 28 01:00 glogg.dmg

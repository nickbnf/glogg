#!/bin/bash

# Build glogg for win32 using the cross-compiler

QTXDIR=$HOME/qt-x-win32

make clean
if [ -z "$VERSION" ]; then
    qmake glogg.pro -spec win32-x-g++ -r CONFIG+="release win32" VERSION="$VERSION"
else
    qmake glogg.pro -spec win32-x-g++ -r CONFIG+="release win32"
fi
make -j3
cp $QTXDIR/qt_win/2010.02.1/qt/bin/{QtCore4,QtGui4}.dll release/
cp $QTXDIR/qt_win/2010.02.1/mingw/bin/libgcc_s_dw2-1.dll release/
gunzip -c /usr/share/doc/mingw32-runtime/mingwm10.dll.gz >release/mingwm10.dll
if [ -z "$VERSION" ]; then
    VERSION=`git describe`;
fi
echo Generating installer for glogg-$VERSION
wine $QTXDIR/NSIS/makensis -DVERSION=$VERSION glogg.nsi

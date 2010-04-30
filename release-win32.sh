#!/bin/bash

PATH=/cygdrive/c/qt/2010.02.1/qt/bin:/cygdrive/c/qt/2010.02.1/mingw/bin/:$PATH
if [ -z "$VERSION" ]; then
    qmake glogg.pro -spec win32-g++ -r CONFIG+=release VERSION="$VERSION"
else
    qmake glogg.pro -spec win32-g++ -r CONFIG+=release
fi
mingw32-make
cp /cygdrive/c/qt/2010.02.1/qt/bin/{QtCore4,QtGui4}.dll release/
cp /cygdrive/c/qt/2010.02.1/mingw/bin/{mingwm10,libgcc_s_dw2-1}.dll release/
if [ -z "$VERSION" ]; then
    VERSION=`git describe`;
fi
echo Generating installer for glogg-$VERSION
/cygdrive/c/Program\ Files/NSIS/makensis -DVERSION=$VERSION glogg.nsi

#!/bin/bash

PATH=/cygdrive/c/qt/2010.02.1/qt/bin:/cygdrive/c/qt/2010.02.1/mingw/bin/:$PATH
qmake glogg.pro -spec win32-g++ -r CONFIG+=release
mingw32-make
cp /cygdrive/c/qt/2010.02.1/qt/bin/{QtCore4,QtGui4}.dll release/
cp /cygdrive/c/qt/2010.02.1/mingw/bin/{mingwm10,libgcc_s_dw2-1}.dll release/
VERSION=`git describe`
echo Generating installer for glogg-$VERSION
/cygdrive/c/Program\ Files/NSIS/makensis -DVERSION=$VERSION glogg.nsi

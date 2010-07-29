#!/bin/bash

BOOSTDIR=$HOME/boost_1_43_0

PATH=/cygdrive/c/qt/2010.02.1/qt/bin:/cygdrive/c/qt/2010.02.1/mingw/bin/:$PATH
if [ -z "$VERSION" ]; then
    echo "Building default version"
    qmake glogg.pro -spec win32-g++ -r CONFIG+=release BOOST_PATH=$BOOSTDIR
else
    echo "Building version $VERSION"
    qmake glogg.pro -spec win32-g++ -r CONFIG+=release BOOST_PATH=$BOOSTDIR VERSION="$VERSION"
fi
mingw32-make
cp /cygdrive/c/qt/2010.02.1/qt/bin/{QtCore4,QtGui4}.dll release/
cp /cygdrive/c/qt/2010.02.1/mingw/bin/{mingwm10,libgcc_s_dw2-1}.dll release/
if [ -z "$VERSION" ]; then
    VERSION=`git describe`;
fi
echo Generating installer for glogg-$VERSION
/cygdrive/c/Program\ Files/NSIS/makensis -DVERSION=$VERSION glogg.nsi

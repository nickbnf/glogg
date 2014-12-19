#!/bin/bash

# Build glogg for win32 using the cross-compiler

QTXDIR=$HOME/qt-x-win32
QTVERSION=4.8.6-32
BOOSTDIR=$QTXDIR/boost_1_50_0

make clean
if [ "$1" == "debug" ]; then
    echo "Building a debug version"
    qmake-qt4 glogg.pro -spec win32-x-g++ -r CONFIG+="debug win32 rtti no-dbus version_checker" BOOST_PATH=$BOOSTDIR
elif [ -z "$VERSION" ]; then
    echo "Building default version"
    qmake-qt4 glogg.pro -spec win32-x-g++ -r CONFIG+="release win32 rtti no-dbus version_checker" BOOST_PATH=$BOOSTDIR
else
    echo "Building version $VERSION"
    qmake-qt4 glogg.pro -spec win32-x-g++ -r CONFIG+="release win32 rtti no-dbus version_checker" BOOST_PATH=$BOOSTDIR VERSION="$VERSION"
fi
make -j3
cp $QTXDIR/$QTVERSION/lib/{QtCore4,QtGui4,QtNetwork4}.dll release/
cp $QTXDIR/$QTVERSION/lib/{QtCored4,QtGuid4,QtNetworkd4}.dll debug/
if [ -z "$VERSION" ]; then
    VERSION=`git describe`;
fi
echo Generating installer for glogg-$VERSION
wine $QTXDIR/NSIS/makensis -DVERSION=$VERSION glogg.nsi

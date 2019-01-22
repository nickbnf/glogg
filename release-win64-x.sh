#!/bin/bash

# Build glogg for win32 using the cross-compiler

QTXDIR=$HOME/qt-x-win32
QTVERSION=5.5.1-64
BOOSTDIR=$QTXDIR/boost_1_50_0

rm release debug .obj .ui .moc -rf
make clean
if [ "$1" == "debug" ]; then
    echo "Building a debug version"
    qmake-qt4 glogg.pro -spec win64-x-g++ -r CONFIG+="debug win32 rtti no-dbus version_checker" BOOST_PATH=$BOOSTDIR
elif [ -z "$VERSION" ]; then
    echo "Building default version"
    qmake glogg.pro -spec win64-x-g++ -r CONFIG+="release win32 rtti no-dbus version_checker" BOOST_PATH=$BOOSTDIR QMAKE_CXXFLAGS="-m64" CROSS_COMPILE="x86_64-w64-mingw32-" INCLUDEPATH+="$QTXDIR/$QTVERSION/include $QTXDIR/$QTVERSION/include/QtCore $QTXDIR/$QTVERSION/include/QtGui $QTXDIR/$QTVERSION/include/QtNetwork $QTXDIR/$QTVERSION/include/QtWidgets"
else
    echo "Building version $VERSION-x86_64"
    qmake glogg.pro -spec win64-x-g++ -r CONFIG+="release win32 rtti no-dbus version_checker" BOOST_PATH=$BOOSTDIR VERSION="$VERSION-x86_64" QMAKE_CXXFLAGS="-m64" CROSS_COMPILE="x86_64-w64-mingw32-" INCLUDEPATH+="$QTXDIR/$QTVERSION/include $QTXDIR/$QTVERSION/include/QtCore $QTXDIR/$QTVERSION/include/QtGui $QTXDIR/$QTVERSION/include/QtNetwork $QTXDIR/$QTVERSION/include/QtWidgets"
fi
make -j3
cp $QTXDIR/$QTVERSION/bin/{Qt5Core,Qt5Gui,Qt5Network,Qt5Widgets}.dll release/
cp $QTXDIR/$QTVERSION/plugins/platforms/qwindows.dll release/
cp $QTXDIR/$QTVERSION/bin/{Qt5Core,Qt5Gui,Qt5Network,Qt5Widgets}d.dll debug/
cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll release/
if [ -z "$VERSION" ]; then
    VERSION=`git describe`;
fi
echo Generating installer for glogg-$VERSION
wine $QTXDIR/NSIS/makensis -DVERSION="$VERSION-x86_64" glogg.nsi

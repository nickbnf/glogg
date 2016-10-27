#!/bin/bash

# Build glogg for OSX and make a DMG installer
# (uses https://github.com/LinusU/node-appdmg)
#
# brew install node
# npm install -g appdmg
#
# QTDIR is built -static

QTDIR=$HOME/Sandbox/qt-5.5.1-release-static
BOOST_PATH=$HOME/Sandbox/boost_1_59_0

make clean
if [ -z "$VERSION" ]; then
    echo Please specify a version to build: VERSION=1.2.3 $0
    exit 1
else
    $QTDIR/qtbase/bin/qmake glogg.pro CONFIG+="release no-dbus version_checker" BOOST_PATH=$BOOSTDIR VERSION="$VERSION"
fi
make -j8

sed -e "s/\"glogg\"/\"glogg $VERSION\"/" osx_installer.json >osx_${VERSION}_installer.json
rm glogg_${VERSION}_installer.dmg
appdmg osx_${VERSION}_installer.json glogg_${VERSION}_installer.dmg
rm osx_${VERSION}_installer.json

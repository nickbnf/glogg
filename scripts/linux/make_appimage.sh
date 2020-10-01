#!/bin/bash
DESTDIR=$(readlink -f appdir) ninja install
wget -c -q "https://github.com/probonopd/linuxdeployqt/releases/download/7/linuxdeployqt-7-x86_64.AppImage"
chmod a+x linuxdeployqt-7-x86_64.AppImage
./linuxdeployqt-7-x86_64.AppImage appdir/usr/share/applications/*.desktop -qmake=$Qt5_Dir/bin/qmake -bundle-non-qt-libs
./linuxdeployqt-7-x86_64.AppImage appdir/usr/share/applications/*.desktop -appimage

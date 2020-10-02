#!/bin/bash
codesign -v -f -o runtime --timestamp -s "Developer ID Application: Anton Filimonov (GAW773U324)" ./output/klogg.app/Contents/MacOS/crash_handler
codesign -v -f -o runtime --timestamp -s "Developer ID Application: Anton Filimonov (GAW773U324)" ./output/klogg.app/Contents/MacOS/minidump_stackwalk

macdeployqt ./output/klogg.app -always-overwrite -verbose=2

python ../3rdparty/macdeployqtfix/macdeployqtfix.py ./output/klogg.app/Contents/MacOS/klogg $(brew --prefix qt5)

codesign -v -f -o runtime --deep --timestamp -s "Developer ID Application: Anton Filimonov (GAW773U324)" ./output/klogg.app;
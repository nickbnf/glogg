#!/bin/bash
macdeployqt ./output/klogg.app -always-overwrite -verbose=2
python ../3rdparty/macdeployqtfix/macdeployqtfix.py ./output/klogg.app/Contents/MacOS/klogg $(brew --prefix qt5)
macdeployqt ./output/klogg.app -dmg -no-plugins

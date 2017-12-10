md %APPVEYOR_BUILD_FOLDER%\release

xcopy %APPVEYOR_BUILD_FOLDER%\build\output\klogg_portable.exe %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %APPVEYOR_BUILD_FOLDER%\build\output\klogg.exe %APPVEYOR_BUILD_FOLDER%\release\ /y

xcopy "%VCToolsRedistDir%x64\Microsoft.VC141.CRT\msvcp140.dll" %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy "%VCToolsRedistDir%x64\Microsoft.VC141.CRT\vcruntime140.dll" %APPVEYOR_BUILD_FOLDER%\release\ /y

xcopy %QT5%\bin\Qt5Core.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %QT5%\bin\Qt5Gui.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %QT5%\bin\Qt5Network.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %QT5%\bin\Qt5Widgets.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %QT5%\bin\Qt5Concurrent.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
md %APPVEYOR_BUILD_FOLDER%\release\platforms
xcopy %QT5%\plugins\platforms\qwindows.dll %APPVEYOR_BUILD_FOLDER%\release\platforms\ /y

xcopy %APPVEYOR_BUILD_FOLDER%\COPYING %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %APPVEYOR_BUILD_FOLDER%\NOTICE %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %APPVEYOR_BUILD_FOLDER%\build\readme.html  %APPVEYOR_BUILD_FOLDER%\release\ /y

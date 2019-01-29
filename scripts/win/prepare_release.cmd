md %APPVEYOR_BUILD_FOLDER%\release

xcopy %APPVEYOR_BUILD_FOLDER%\build\output\klogg_portable.exe %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %APPVEYOR_BUILD_FOLDER%\build\output\klogg.exe %APPVEYOR_BUILD_FOLDER%\release\ /y

xcopy "%VCToolsRedistDir%x64\Microsoft.VC141.CRT\msvcp140.dll" %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy "%VCToolsRedistDir%x64\Microsoft.VC141.CRT\vcruntime140.dll" %APPVEYOR_BUILD_FOLDER%\release\ /y

xcopy %APPVEYOR_BUILD_FOLDER%\build\output\klogg_tbbmalloc.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %APPVEYOR_BUILD_FOLDER%\build\output\klogg_tbbmalloc_proxy.dll %APPVEYOR_BUILD_FOLDER%\release\ /y

xcopy %QTDIR%\bin\Qt5Core.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %QTDIR%\bin\Qt5Gui.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %QTDIR%\bin\Qt5Network.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %QTDIR%\bin\Qt5Widgets.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %QTDIR%\bin\Qt5Concurrent.dll %APPVEYOR_BUILD_FOLDER%\release\ /y
md %APPVEYOR_BUILD_FOLDER%\release\platforms
xcopy %QTDIR%\plugins\platforms\qwindows.dll %APPVEYOR_BUILD_FOLDER%\release\platforms\ /y

xcopy %APPVEYOR_BUILD_FOLDER%\COPYING %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %APPVEYOR_BUILD_FOLDER%\NOTICE %APPVEYOR_BUILD_FOLDER%\release\ /y
xcopy %APPVEYOR_BUILD_FOLDER%\build\readme.html  %APPVEYOR_BUILD_FOLDER%\release\ /y

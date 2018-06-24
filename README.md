[![Build Status](https://travis-ci.org/variar/klogg.svg?branch=master)](https://travis-ci.org/variar/klogg)
[![Win32 Build Status](https://ci.appveyor.com/api/projects/status/github/variar/klogg?svg=true)](https://ci.appveyor.com/project/variar/klogg)
[ ![Download](https://api.bintray.com/packages/variar/generic/klogg/images/download.svg) ](https://bintray.com/variar/generic/klogg/_latestVersion)

CI builds 18.06.0.259 (x64):
[windows portable](https://ci.appveyor.com/api/buildjobs/uvh5b4r8cslc8hlh/artifacts/klogg-18.06.0.259-portable.zip) |
[windows installer](https://ci.appveyor.com/api/buildjobs/uvh5b4r8cslc8hlh/artifacts/klogg-18.06.0.259-setup.exe) |
[deb](https://s3.amazonaws.com/klogg.travis.build/variar/klogg/259/259.1/build/packages/klogg-18.6.0-r259-Linux.deb) |
[rpm](https://s3.amazonaws.com/klogg.travis.build/variar/klogg/259/259.1/build/packages/klogg-18.6.0-r259-Linux.rpm) |
[appimage](https://s3.amazonaws.com/klogg.travis.build/variar/klogg/259/259.1/build/packages/klogg-18.06.0.259-x86_64.AppImage) |
[dmg](https://s3.amazonaws.com/klogg.travis.build/variar/klogg/259/259.2/build/packages/klogg-18.6.0-r259-OSX.dmg)

klogg is the fork of [glogg](https://github.com/nickbnf/glogg) - the fast, smart log explorer.

From original glogg description:
> glogg is a multi-platform GUI application that helps browse and search
> through long and complex log files.  It is designed with programmers and
> system administrators in mind and can be seen as a graphical, interactive
> combination of grep and less.



## Common features of klogg and glogg
* Runs on Unix-like systems, Windows and Mac thanks to Qt5
* Search results are displayed separately from original file
* Supports Perl-compatible regular expressions 
* Colorizes the log and search results
* Displays a context view of where in the log the lines of interest are
* Is fast and reads the file directly from disk, without loading it into memory
* Watches for file changes on disk and reloads it (kind of like tail)
* Is open source, released under the GPL

## Features of klogg
* Log encoding detection using [uchardet](https://www.freedesktop.org/wiki/Software/uchardet/) library (support utf8, utf16, cp1251 and more)
* Limiting search to a part of open file
* In-memory cache of search results per search pattern
* Parallel regular expression matching (speed up search on multicore CPUs)
* Watching for file changes using [Entropia File System Watcher](https://bitbucket.org/SpartanJ/efsw) on all platforms
* Using [cli11](https://github.com/CLIUtils/CLI11) for command line parsing (no dependency on program_options library from boost)

Latest win32 builds from master branch can be downloaded from Appveyor:
https://ci.appveyor.com/project/variar/klogg/build/artifacts?branch=master

Linux and Mac OS builds are uploaded from Travis CI to Amazon S3 (S3 browser software is required): https://s3.amazonaws.com/klogg.travis.build/

## Building from source
Unlike upstream klogg is using cmake to generate build files.

### Requirements

* cmake 3.2 or later
* C++ compiler with C++14 support (gcc 5, clang 3.4, msvc 2015)
* Qt libraries (version 5.7 or later). QtCore, QtGui, QtWidgets and QtConcurrent are required on all platforms. QtNetwork is required on Windows and Mac OS. On Linux either QtNetwork or QtDBus can be used (selected during build configuration). QtTest is needed to build and run tests.
* pandoc to build documentation

All other build dependencies are provided in 3rdparty directory. 

### Building on Linux and Mac
```
cd <path_to_project_root>
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_VERSION=X.X.X -DCMAKE_INSTALL_PREFIX=/usr/local ../
cmake --build .
make install (as root if needed)
```

Binaries are placed into `build/output`.

Deb or rpm packages can be built with cpack. Run cpack in build directory:
```
cpack -G DEB
cpack -G RPM
```

Packages will be placed into `build/packages`.

See `.travis.yml` for more information on build process.

### Building on Windows
Assuming `QT5` environment variable contains full path to Qt installation root folder and
Visual Studio 2017 Community Edition is used for C++ compiler (vsdevcmd batch file location depends on Visual Studio version):
```
%QT5%\bin\qtenv2.bat
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\vsdevcmd" -arch=x64
cd <path_to_project_root>
md build
cd build
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_VERSION=X.X.X ../
cmake --build . --config Release
```

Change cmake generator to "Visual Studio 14 2015" (or "Visual Studio 15 2017" with recent cmake) to get solution files and build from IDE.

Installer can be built with NSIS (requires documentation to be built). 
```
cd <path_to_project_root>
md release
xcopy build\output\klogg.exe release\ /y
xcopy build\output\readme.html release\ /y
xcopy %QT5%\bin\Qt5Core.dll release\ /y
xcopy %QT5%\bin\Qt5Gui.dll release\ /y
xcopy %QT5%\bin\Qt5Network.dll release\ /y
xcopy %QT5%\bin\Qt5Widgets.dll release\ /y
xcopy %QT5%\bin\Qt5Concurrent.dll release\ /y
md release\platforms
xcopy %QT5%\plugins\platforms\qwindows.dll release\platforms\ /y
xcopy "%VCToolsRedistDir%x64\Microsoft.VC141.CRT\msvcp140.dll" release\ /y
xcopy "%VCToolsRedistDir%x64\Microsoft.VC141.CRT\vcruntime140.dll" release\ /y

makensis -DVERSION=X.X.X klogg.nsi
```

See `appveyor.yml` for more information on build process.

### Tests
Test are built by default. To turn them off pass `-DBUILD_TESTS:BOOL=OFF` to cmake.
Tests use google test and google mock (they are bundled with the project source) and require Qt5Test module. 


## Contact
You can reach out to me on github.

Please visit glogg's website: http://glogg.bonnefon.org/

The glogg development mailing list is hosted at http://groups.google.co.uk/group/glogg-devel

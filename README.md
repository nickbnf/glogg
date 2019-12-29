| CI | Windows | Linux  | Mac |
| ------------- |------------- | ------------- | ------------- |
| Build status | [![Win32 Build Status](https://ci.appveyor.com/api/projects/status/github/variar/klogg?svg=true)](https://ci.appveyor.com/project/variar/klogg) | [![Build Status](https://travis-ci.org/variar/klogg.svg?branch=master)](https://travis-ci.org/variar/klogg) | [![Build Status](https://travis-ci.org/variar/klogg.svg?branch=master)](https://travis-ci.org/variar/klogg) |
| Artifacts | [continuous-win](https://github.com/variar/klogg/releases/tag/continuous-win) | [continuous-linux](https://github.com/variar/klogg/releases/tag/continuous-linux) | [continuous-osx](https://github.com/variar/klogg/releases/tag/continuous-osx) | 

### Current release

[ ![Release](https://img.shields.io/github/v/release/variar/klogg?style=flat)](https://github.com/variar/klogg/releases/tag/v19.9)
[ ![Bintray](https://img.shields.io/bintray/v/variar/generic/klogg?style=flat)](https://bintray.com/variar/generic/klogg/_latestVersion)
[ ![Chocolatey](https://img.shields.io/chocolatey/v/klogg?style=flat)](https://chocolatey.org/packages/klogg)


Klogg is a multi-platform GUI application that helps browse and search
through long and complex log files. It is designed with programmers and
system administrators in mind and can be seen as a graphical, interactive
combination of grep, less and tail. 

## Table of Contents

1. [About the Project](#about-the-project)
1. [Project Status](#project-status)
1. [Getting Started](#getting-started)
    1. [Dependencies](#dependencies)
    1. [Building](#building)
    1. [Runnung tests](#running-tests)
1. [How to Get Help](#how-to-get-help)
1. [Contributing](#contributing)
1. [License](#license)
1. [Authors](#authors)

# About the Project

Klogg started as a fork of [glogg](https://github.com/nickbnf/glogg) - the fast, smart log explorer in 2016.

Since then it has evolved from fixing small annoying bugs to rewriting core components to
make it faster and smarter that predecessor.

I use klogg every day and development is primarily focused on feature my colleagues and I need
to stay productive as well as feature requests from users on Github and in glogg mailing list.

Latest news about klogg development can be found at https://klogg.filimonov.dev.

## Common features of klogg and glogg
* Runs on Unix-like systems, Windows and Mac thanks to Qt5
* Is fast and reads the file directly from disk, without loading it into memory
* Can operate on huge text files (10+ Gb is not a problem)
* Search results are displayed separately from original file
* Supports Perl-compatible regular expressions 
* Colorizes the log and search results
* Displays a context view of where in the log the lines of interest are
* Watches for file changes on disk and reloads it (kind of like tail)
* Is open source, released under the GPL

## Features of klogg
* Multithreading support for file indexing and regular expression matching
* Log encoding detection using [uchardet](https://www.freedesktop.org/wiki/Software/uchardet/) library (support utf8, utf16, cp1251 and more)
* Limiting search to a part of open file
* In-memory cache of search results per search pattern
* Support for many common text encodings
* And lots of small features that make life easier (closing tabs, copying file paths, favorite files menu, etc.)

**[Back to top](#table-of-contents)**

# Project Status

This project uses [Calendar Versioning](https://calver.org/). For a list of available versions, see the [repository tag list](https://github.com/variar/klogg/tags).

### Current release builds

Current release is 19.9. Binaries for all platforms can be downloaded from GitHub releases or Bintray.

[ ![Release](https://img.shields.io/github/v/release/variar/klogg?style=flat)](https://github.com/variar/klogg/releases/tag/v19.9)
[ ![Bintray](https://img.shields.io/bintray/v/variar/generic/klogg?style=flat)](https://bintray.com/variar/generic/klogg/_latestVersion)

Windows installer is also available from Chocolatey:

[ ![Chocolatey](https://img.shields.io/chocolatey/v/klogg?style=flat)](https://chocolatey.org/packages/klogg)

### CI status

| | Windows | Linux  | Mac |
| ------------- |------------- | ------------- | ------------- |
| Build status | [![Win32 Build Status](https://ci.appveyor.com/api/projects/status/github/variar/klogg?svg=true)](https://ci.appveyor.com/project/variar/klogg) | [![Build Status](https://travis-ci.org/variar/klogg.svg?branch=master)](https://travis-ci.org/variar/klogg) | [![Build Status](https://travis-ci.org/variar/klogg.svg?branch=master)](https://travis-ci.org/variar/klogg) |
| Artifacts | [continuous-win](https://github.com/variar/klogg/releases/tag/continuous-win) | [continuous-linux](https://github.com/variar/klogg/releases/tag/continuous-linux) | [continuous-osx](https://github.com/variar/klogg/releases/tag/continuous-osx) | 

**[Back to top](#table-of-contents)**

# Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

## Dependencies

* cmake 3.12 or later to generate build files
* C++ compiler with C++14 support (gcc 5, clang 3.4, msvc 2017)
* Qt libraries 5.9 or later:
    - QtCore
    - QtGui
    - QtWidgets
    - QtConcurrent
    - QtNetwork
    - QtXml
    - QtTest
* pandoc to build documentation (optional)
* nsis to build windows installer (optional)

All other dependencies are provided in 3rdparty directory. 

## Getting the Source

This project is [hosted on GitHub](https://github.com/variar/klogg). You can clone this project directly using this command:
```
git clone https://github.com/variar/klogg
```

## Building

### Building on Linux

Here is how to build klogg on Ubuntu 18.04. 

Install dependencies:
```
sudo apt-get install build-essential cmake qtbase5-dev 
```

Configure and build klogg:

```
cd <path_to_klogg_repository_clone>
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_DOC=False ..
cmake --build .
```

Binaries are placed into `build/output`.

See `.travis.yml` for more information on build process.

### Building on Windows

Install Microsoft Visual Studio 2017 or 2019 with C++ support.
Community edition can be downloaded from [Microsoft](https://visualstudio.microsoft.com/vs/). 

Intall latest Qt version using [online installer](https://www.qt.io/download-qt-installer).
Make sure to select version matching Visual Studio installation. 64-bit libraries are recommended.

Install CMake from [Kitware](https://cmake.org/download/). 
Use version 3.14 or later for Visual Studio 2019 support.

Prepare build environment for CMake. Open command prompt window and depending on version of Visual Studio run either
```
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\vsdevcmd" -arch=x64
```
or
```
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\vsdevcmd" -arch=x64
```

Next setup Qt paths:
```
<path_to_qt_installation>\bin\qtenv2.bat
```

Then add CMake to PATH:
```
set PATH=<path_to_cmake_bin>:$PATH
```

Configure klogg solution (use CMake generator matching Visual Studio version):
```
cd <path_to_project_root>
md build
cd build
cmake -G "Visual Studio 16 2019 Win64" -DCMAKE_BUILD_TYPE=Release -DBUILD_DOC=False ..
```

CMake should generate `klogg.sln` file in `<path_to_project_root>\build` directory. Open solution and build it.

Binaries are placed into `build/output`.

### Building on Mac OS

Klogg requires macOS High Sierra (10.13) or higher.

Install [Homebrew](https://brew.sh/) using terminal:
```
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Homebrew installer should also install xcode command line tools.

Download and install build dependencies:
```
brew install cmake ninja qt
```

Configure and build klogg:
```
cd <path_to_klogg_repository_clone>
mkdir build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_DOC=False -DQt5_DIR=/usr/local/Cellar/qt/5.14.0/lib/cmake/Qt5 ..
cmake --build .
```
 
Binaries are placed into `build/output`.
 
## Running tests
Tests are built by default. To turn them off pass `-DBUILD_TESTS:BOOL=OFF` to cmake.
Tests use catch2 (bundled with klogg sources) and require Qt5Test module. Tests can be run using ctest tool provider by CMake:
```
cd <path_to_klogg_repository_clone>
cd build
ctest --build-config Release --verbose
```

**[Back to top](#table-of-contents)**


# How to Get Help
You can open issues using [klogg issues page](https://github.com/variar/klogg/issues) 
or post questions to glogg development [mailing list](http://groups.google.co.uk/group/glogg-devel).

# Contributing

We encourage public contributions! Please review [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct and development process.

# License

This project is licensed under the GPLv3 or later - see [COPYING](COPYING) file for details.

# Authors

* **[Anton Filimonov](https://github.com/variar)** 
* *Initial work* - **[Nicolas Bonnefon](https://github.com/nickbnf)**

See also the list of [contributors](https://klogg.filimonov.dev/docs/getting_involved/#contributors) who participated in this project.

**[Back to top](#table-of-contents)**


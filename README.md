[![Build Status](https://travis-ci.org/variar/klogg.svg?branch=master)](https://travis-ci.org/variar/klogg)
 [![Win32 Build Status](https://ci.appveyor.com/api/projects/status/github/variar/klogg?svg=true)](https://ci.appveyor.com/project/variar/klogg)

klogg is the fork of glogg - the fast, smart log explorer.

The main reason for this fork is that we use glogg a lot
and feel that usability can be improved here and there.

## Features of klogg
* Automatic log encoding detection using uchardet library
* Limiting search to a part of open file
* In-memory cache of search results per search pattern
* Parallel pattern matching using all available cpu cores

Dependency on boost "program-options" library has been removed.

Latest win32 builds from master branch can be downloaded from Appveyor:
https://ci.appveyor.com/project/variar/klogg/build/artifacts?branch=master

## About original glogg
=======

glogg - the fast, smart log explorer
=====================================

glogg is a multi-platform GUI application that helps browse and search
through long and complex log files.  It is designed with programmers and
system administrators in mind and can be seen as a graphical, interactive
combination of grep and less.

## Main features

* Runs on Unix-like systems, Windows and Mac thanks to Qt
* Provides a second window showing the result of the current search
* Reads UTF-8 and ISO-8859-1 files
* Supports grep/egrep like regular expressions
* Colorizes the log and search results
* Displays a context view of where in the log the lines of interest are
* Is fast and reads the file directly from disk, without loading it into memory
* Is open source, released under the GPL

## Requirements

* GCC version 4.8.0 or later
* Qt libraries (version 5.2.0 or later)
* Boost "program-options" development libraries
* Markdown HTML processor (optional, to generate HTML documentation)

glogg version 0.9.X still support older versions of gcc and Qt if you need to
build on an older platform.

## Building

The build system uses qmake. Building and installation is done this way:

```
tar xzf glogg-X.X.X.tar.gz
cd glogg-X.X.X
qmake
make
make install INSTALL_ROOT=/usr/local (as root if needed)
```

`qmake BOOST_PATH=/path/to/boost/` will statically compile the required parts of
the Boost libraries whose source are found at the specified path.
The path should be the directory where the tarball from www.boost.org is
extracted.
(use this method on Windows or if Boost is not available on the system)

The documentation is built and installed automatically if 'markdown'
is found.

## Tests

The tests are built using CMake, and require Qt5 and the Google Mocks source.

```
cd tests
mkdir build
cd build
export QT_DIR=/path/to/qt/if/non/standard
export GMOCK_HOME=/path/to/gmock
cmake ..
make
./glogg_tests
```

## Contact

Please visit glogg's website: http://glogg.bonnefon.org/

The development mailing list is hosted at http://groups.google.co.uk/group/glogg-devel

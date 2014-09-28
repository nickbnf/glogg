# Generate makefiles suitable to make the tests on Windows/MinGW

mkdir build
cd build

set QT_DIR=C:\qt-everywhere-opensource-src-5.3.2\qtbase
set CMAKE_PREFIX_PATH=%QT_DIR%
set GMOCK_HOME=C:\gmock-1.7.0

cmake -G "MinGW Makefiles" ..

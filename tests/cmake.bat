rem Generate makefiles suitable to make the tests on Windows/MinGW

mkdir build
cd build

set QT_DIR=Y:\5.3.2-64
set CMAKE_PREFIX_PATH=%QT_DIR%
set GMOCK_HOME=Y:\gmock-1.7.0
set QT_QPA_PLATFORM_PLUGIN_PATH=%QT_DIR%\plugins\platforms

cmake -G "MinGW Makefiles" ..

mingw32-make

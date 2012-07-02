# Building glogg for Windows

As often on Windows, building _glogg_ is more complicated than we would like.
The method we use to test and generate the builds available on
glogg.bonnefon.org is to cross-build _glogg_ from a Debian/Ubuntu machine.

The compiler currently used is MinGW-W64 4.6.3, which is available on Debian unstable and Ubuntu 12.04 LTS:

    apt-get install g++-mingw-w64-i686

There are two dependencies that must be built for Windows:
 - Qt, we currently use version 4.8.2
 - Boost, we currently use version 1.50.0

Once the dependencies are installed (see below), a script takes care of the building and the packaging:

    ./release-win32-x.sh

## Building Qt on Windows

It is allegedly possible to cross-compile Qt from Windows but after a lot of
frustration, the _glogg_ team is building it natively on Windows (using a
Windows 7 64bit VM in VirtualBox) and then using the binary generated from the
Linux machine.  Amazingly, it works as long as we are using a similar version
of gcc (MinGW-W64) on both machines.

Here are instructions to do the build in a Windows 7 VM:

 - Download the source from qt website (tested 4.8.2), be sure to download it with DOS end-of-line conventions (zip archive instead of tar.bz2 or tar.gz).
 - Install mingw-w64 from TDM-GCC (tdm64-gcc, tested with 4.6.1).
 - Extract the Qt source in c:\qt\4.8.2

If building a 32 bits version of Qt (what we do for _glogg_):

 - Modify qt/4.8.2/mkspecs/win32-g++/qmake.conf to add `-m32` to `QMAKE_CCFLAGS` and `QMAKE_LFLAGS`
 - Modify qt/4.8.2/mkspecs/win32-g++/qmake.conf to replace: `QMAKE_RC = windres -F pe-i386`
 - (optionally make other changes here to improve performances)

Build from the MinGW command prompt:

    configure.exe -platform win32-g++-4.6 -no-phonon -no-phonon-backend -no-webkit -fast -opensource -shared -no-qt3support -no-sql-sqlite -no-openvg -no-gif -no-opengl -no-scripttools
    mingw32-make

 - copy the whole `qt/4.8.2` to the linux machine in `~/qt-x-win32/qt_win/4.8.2`

### Creating the cross-compiled Qt target

 - Install ~/qt-x-win32 created before

Then create a copy of the standard gcc target to create the new cross-gcc target:

    cd /usr/share/qt4/mkspecs/
    sudo cp -a win32-g++ win32-x-g++

and modify it to point to the cross-compiler and our local version of Windows Qt:

    sudo sed -i -re 's/ (gcc|g\+\+)/ i686-w64-mingw32-\1/' win32-x-g++/qmake.conf
    sudo sed -i -re '/QMAKE_SH/iQMAKE_SH=1' win32-x-g++/qmake.conf
    sudo sed -i -re 's/QMAKE_COPY_DIR.*$/QMAKE_COPY_DIR = cp -r/' win32-x-g++/qmake.conf
    sudo sed -i -re '/QMAKE_LFLAGS/s/$/ -mwindows/' win32-x-g++/qmake.conf
    sudo sed -i -re 's/QMAKE_RC.*$/QMAKE_RC = i686-w64-mingw32-windres/' win32-x-g++/qmake.conf
    sudo sed -i -re 's/QMAKE_STRIP\s.*$/QMAKE_STRIP = i686-w64-mingw32-strip/' win32-x-g++/qmake.conf
    sudo sed -i -re 's/\.exe//' win32-x-g++/qmake.conf
    sudo sed -i -re 's/QMAKE_INCDIR\s.*$/QMAKE_INCDIR = \/usr\/i686-w64-mingw32\/include/' win32-x-g++/qmake.conf
    sudo sed -i -re 's/QMAKE_INCDIR_QT\s.*$/QMAKE_INCDIR_QT = \/home\/$USER\/qt_win\/4.8.2\/include/' win32-x-g++/qmake.conf
    sudo sed -i -re 's/QMAKE_LIBDIR_QT\s.*$/QMAKE_LIBDIR_QT = \/home\/$USER\/qt_win\/4.8.2\/lib/' win32-x-g++/qmake.conf

Now you can build a hello world to test Qt:

    mkdir /tmp/testqt
    cd /tmp/testqt
    echo '#include <QApplication>
     #include <QPushButton>

     int main(int argc, char *argv[])
     {
         QApplication app(argc, argv);

         QPushButton hello("Hello world!");
         hello.resize(100, 30);

         hello.show();
         return app.exec();
     }' >main.cpp
    qmake -project
    qmake -spec win32-x-g++ -r CONFIG+=release
    make

## Building Boost on Windows

Download the source from boost.org (tested with 1.50.0), DOS EOL mandatory!

Extract it in a Windows VM

Edit bootstrap.bat to read:

    call .\build.bat mingw %*...
    set toolset=gcc

And from the MinGW prompt:

    bootstrap
    b2 toolset=gcc address-model=32 variant=debug,release link=static,shared threading=multi install

 - Copy the whole c:\boost_1_50_0 to the linux machine to ~/qt-x-win32/boost_1_50_0

## (optional) Install NSIS

If _wine_ and the NSIS compiler (available from [here](http://nsis.sourceforge.net/Main_Page)) are available, the script will generate the installer for _glogg_.

The NSIS compiler should be installed in `~/qt-x-win32/NSIS`.

## Building _glogg_

From this point, building _glogg_ is hopefully straightforward:

    ./release-win32-x.sh

The `release-win32-x.sh` script might need some changes if you use different paths for the dependencies.

The object file is in `./release/`

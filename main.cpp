#include <QApplication>

#include "mainwindow.h"
#include "log.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow mw;

    LOG(logDEBUG) << "MainWindow created.";
    mw.show();
    return app.exec();
}

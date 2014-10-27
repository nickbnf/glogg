#include "gmock/gmock.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a( argc, argv );
    ::testing::InitGoogleTest(&argc, argv);
    int iReturn = RUN_ALL_TESTS();

    // qDebug()<<"rcode:"<<iReturn;

    return a.exec();
}

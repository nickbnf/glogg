#include "gmock/gmock.h"

#include <QApplication>

#include "persistentinfo.h"
#include "configuration.h"

int main(int argc, char *argv[]) {
    QApplication a( argc, argv );
    ::testing::InitGoogleTest(&argc, argv);

    // Register the configuration items
    GetPersistentInfo().migrateAndInit();
    GetPersistentInfo().registerPersistable(
     std::make_shared<Configuration>(), QString( "settings" ) );

    int iReturn = RUN_ALL_TESTS();
    Q_UNUSED(iReturn);

    // qDebug()<<"rcode:"<<iReturn;

    return a.exec();
}

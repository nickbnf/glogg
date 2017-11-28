#include "gmock/gmock.h"

#include <QApplication>
#include <QtConcurrent>

#include "persistentinfo.h"
#include "configuration.h"

int main(int argc, char *argv[]) {
    QApplication a( argc, argv );
    ::testing::InitGoogleTest(&argc, argv);

    // Register the configuration items
    GetPersistentInfo().migrateAndInit();
    GetPersistentInfo().registerPersistable(
     std::make_shared<Configuration>(), QString( "settings" ) );

    QtConcurrent::run([&a]()
    {
        int result = RUN_ALL_TESTS();
        a.processEvents();
        a.exit(result);
    });

    return a.exec();
}

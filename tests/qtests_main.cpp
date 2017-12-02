#include "gmock/gmock.h"

#include <QApplication>
#include <QtConcurrent>
#include <QMetaType>

#include "persistentinfo.h"
#include "configuration.h"
#include "data/linetypes.h"

#include <log.h>
#include <plog/Appenders/ConsoleAppender.h>

int main(int argc, char *argv[]) {
    QApplication a( argc, argv );
    ::testing::InitGoogleTest(&argc, argv);

    plog::ConsoleAppender<plog::GloggFormatter> appender;
    plog::init(logINFO, &appender);

    // Register the configuration items
    GetPersistentInfo().migrateAndInit();
    GetPersistentInfo().registerPersistable(
     std::make_shared<Configuration>(), QString( "settings" ) );


    qRegisterMetaType<LinesCount>( "LinesCount" );
    qRegisterMetaType<LineNumber>( "LineNumber" );
    qRegisterMetaType<LineLength>( "LineLength" );

    QtConcurrent::run([&a]()
    {
        int result = RUN_ALL_TESTS();
        a.processEvents();
        a.exit(result);
    });

    return a.exec();
}

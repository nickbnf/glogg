#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <QApplication>
#include <QtConcurrent>
#include <QMetaType>

#include <persistentinfo.h>
#include <configuration.h>
#include <data/linetypes.h>

#include <log.h>
#include <plog/Appenders/ConsoleAppender.h>

int main(int argc, char *argv[]) {
    QApplication a( argc, argv );

    plog::ConsoleAppender<plog::GloggFormatter> appender;
    plog::init( logINFO, &appender );

    // Register the configuration items
    GetPersistentInfo().migrateAndInit( PersistentInfo::Portable );
    GetPersistentInfo().registerPersistable(
     std::make_unique<Configuration>(), "settings" );


    qRegisterMetaType<LinesCount>( "LinesCount" );
    qRegisterMetaType<LineNumber>( "LineNumber" );
    qRegisterMetaType<LineLength>( "LineLength" );

    auto& config = Persistent<Configuration>( "settings" );
    config.setSearchReadBufferSizeLines(10);

#ifdef Q_OS_WIN
    config.setPollingEnabled(true);
    config.setPollIntervalMs(1000);
#else
    config.setPollingEnabled(false);
#endif

    QtConcurrent::run( [&a, &argc, &argv]()
    {
        int result = Catch::Session().run( argc, argv );
        a.processEvents();
        a.exit(result);
    });

    return a.exec();
}

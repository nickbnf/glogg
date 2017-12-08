#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <log.h>
#include <plog/Appenders/ConsoleAppender.h>

int main(int argc, char *argv[]) {

    plog::ConsoleAppender<plog::GloggFormatter> appender;
    plog::init( logWARNING, &appender );

    return Catch::Session().run( argc, argv );
}

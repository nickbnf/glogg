#include "gmock/gmock.h"

#include <log.h>
#include <plog/Appenders/ConsoleAppender.h>

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);

    plog::ConsoleAppender<plog::GloggFormatter> appender;
    plog::init(logDEBUG, &appender);

    return RUN_ALL_TESTS();
}

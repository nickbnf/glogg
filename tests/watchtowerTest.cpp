#include "gmock/gmock.h"

#include <fcntl.h>

#include <memory>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <chrono>

#include "inotifywatchtower.h"

using namespace std;
using namespace testing;

class WatchTowerBehaviour: public testing::Test {
  public:
    INotifyWatchTower watch_tower;

    void SetUp() override {
#ifdef _WIN32
        file_watcher = make_shared<WinFileWatcher>();
#endif
    }

    string createTempEmptyFile() {
        // I know tmpnam is bad but I need control over the file
        // and it is the only one which exits on Windows.
        char* file_name = tmpnam( nullptr );
        int fd = creat( file_name, S_IRUSR | S_IWUSR );
        close( fd );

        return string( file_name );
    }

    string getNonExistingFileName() {
        return string( tmpnam( nullptr ) );
    }
};

TEST_F( WatchTowerBehaviour, AcceptsAnExistingFileToWatch ) {
    auto file_name = createTempEmptyFile();
    auto registration = watch_tower.addFile( file_name, [] (void) { } );
}

TEST_F( WatchTowerBehaviour, AcceptsANonExistingFileToWatch ) {
    auto registration = watch_tower.addFile( getNonExistingFileName(), [] (void) { } );
}

class WatchTowerSingleFile: public WatchTowerBehaviour {
  public:
    static const int TIMEOUT = 100;

    string file_name;
    INotifyWatchTower::Registration registration;

    mutex mutex_;
    condition_variable cv_;
    bool notification_received = false;

    void SetUp() override {
        file_name = createTempEmptyFile();
        registration = watch_tower.addFile( file_name, [this] (void) {
            unique_lock<mutex> lock(mutex_);
            notification_received = true;
            cv_.notify_one(); } );
    }

    bool waitNotificationReceived() {
        unique_lock<mutex> lock(mutex_);
        return cv_.wait_for( lock, std::chrono::milliseconds(100),
                [this] { return notification_received; } );
    }

    void appendDataToFile( const string& file_name ) {
        static const char* string = "Test line\n";
        int fd = open( file_name.c_str(), O_WRONLY | O_APPEND );
        write( fd, (void*) string, strlen( string ) );
        close( fd );
    }
};

TEST_F( WatchTowerSingleFile, SignalsWhenAWatchedFileIsAppended ) {
    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( WatchTowerSingleFile, StopSignalingWhenWatchDeleted ) {
    remove( file_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );
}

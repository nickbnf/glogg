#include "gmock/gmock.h"

#include <cstdio>
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

    string createTempEmptyFile( string file_name = "" ) {
        const char* name;

        if ( ! file_name.empty() ) {
            name = file_name.c_str();
        }
        else {
            // I know tmpnam is bad but I need control over the file
            // and it is the only one which exits on Windows.
            name = tmpnam( nullptr );
        }
        int fd = creat( name, S_IRUSR | S_IWUSR );
        close( fd );

        return string( name );
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

/*****/

class WatchTowerSingleFile: public WatchTowerBehaviour {
  public:
    static const int TIMEOUT;

    string file_name;
    INotifyWatchTower::Registration registration;

    mutex mutex_;
    condition_variable cv_;
    bool notification_received = false;

    INotifyWatchTower::Registration registerFile( const string& file_name ) {
        weak_ptr<void> weakHeartbeat( heartbeat_ );

        auto reg = watch_tower.addFile( file_name, [this, weakHeartbeat] (void) {
            // Ensure the fixture object is still alive using the heartbeat
            if ( auto keep = weakHeartbeat.lock() ) {
                unique_lock<mutex> lock(mutex_);
                notification_received = true;
                cv_.notify_one();
            } } );

        return reg;
    }

    WatchTowerSingleFile() : WatchTowerBehaviour(),
        heartbeat_( shared_ptr<void>( (void*) 0xDEADC0DE, [] (void*) {} ) )
    { }

    void SetUp() override {
        file_name = createTempEmptyFile();
        registration = registerFile( file_name );
    }

    bool waitNotificationReceived() {
        unique_lock<mutex> lock(mutex_);
        bool success = cv_.wait_for( lock, std::chrono::milliseconds(TIMEOUT),
                [this] { return notification_received; } );

        // Reinit the notification
        notification_received = false;

        return success;
    }

    void appendDataToFile( const string& file_name ) {
        static const char* string = "Test line\n";
        int fd = open( file_name.c_str(), O_WRONLY | O_APPEND );
        write( fd, (void*) string, strlen( string ) );
        close( fd );
    }

    void TearDown() override {
        remove( file_name.c_str() );
    }

  private:
    // Heartbeat ensures the object is still alive
    shared_ptr<void> heartbeat_;
};

const int WatchTowerSingleFile::TIMEOUT = 20;

TEST_F( WatchTowerSingleFile, SignalsWhenAWatchedFileIsAppended ) {
    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( WatchTowerSingleFile, SignalsWhenAWatchedFileIsRemoved) {
    remove( file_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( WatchTowerSingleFile, SignalsWhenADeletedFileReappears ) {
    remove( file_name.c_str() );
    waitNotificationReceived();
    createTempEmptyFile( file_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( WatchTowerSingleFile, StopSignalingWhenWatchDeleted ) {
    auto second_file_name = createTempEmptyFile();
    {
        auto second_registration = registerFile( second_file_name );
        appendDataToFile( second_file_name );
        ASSERT_TRUE( waitNotificationReceived() );
    }

    // The registration will be removed here.
    appendDataToFile( second_file_name );
    ASSERT_FALSE( waitNotificationReceived() );

    remove( second_file_name.c_str() );
}

TEST( WatchTowerLifetime, RegistrationCanBeDeletedWhenWeAreDead ) {
    auto mortal_watch_tower = new INotifyWatchTower();
    auto reg = mortal_watch_tower->addFile( "/tmp/test_file", [] (void) { } );

    delete mortal_watch_tower;
}

#include "gmock/gmock.h"

#include "config.h"

#include <cstdio>
#include <fcntl.h>

#include <memory>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <chrono>

#ifdef _WIN32
#  include "winwatchtower.h"
#else
#  include "inotifywatchtower.h"
#endif

using namespace std;
using namespace testing;

#ifdef _WIN32
#if 0
TEST( WatchTowerBase, NotifyAFile ) {
    int fd = creat( "c:\\test", S_IRUSR | S_IWUSR );
    close( fd );

    auto watch_tower = make_shared<WinWatchTower>();
    std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
    {
        auto registration = watch_tower->addFile( "c:\\test", [] {} );

        std::this_thread::sleep_for( std::chrono::milliseconds(5000) );

        static const char* string = "Test line\n";
        fd = open( "c:\\test", O_WRONLY | O_APPEND );
        write( fd, (void*) string, strlen( string ) );
        close( fd );

        std::this_thread::sleep_for( std::chrono::milliseconds(10000) );
    }
}
#endif
#endif

class WatchTowerBehaviour: public testing::Test {
  public:
#ifdef _WIN32
    shared_ptr<WatchTower> watch_tower = make_shared<WinWatchTower>();
#else
    shared_ptr<WatchTower> watch_tower = make_shared<INotifyWatchTower>();
#endif

    string createTempEmptyFile( string file_name = "" ) {
        const char* name;

        if ( ! file_name.empty() ) {
            name = file_name.c_str();
        }
        else {
            // I know tmpnam is bad but I need control over the file
            // and it is the only one which exits on Windows.
#if _WIN32
            name = _tempnam( "c:\\temp", "glogg_test" );
#else
            name = tmpnam( nullptr );
#endif
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
    auto registration = watch_tower->addFile( file_name, [] (void) { } );
}

TEST_F( WatchTowerBehaviour, AcceptsANonExistingFileToWatch ) {
    auto registration = watch_tower->addFile( getNonExistingFileName(), [] (void) { } );
}

/*****/

class WatchTowerSingleFile: public WatchTowerBehaviour {
  public:
    static const int TIMEOUT;

    string file_name;
    WatchTower::Registration registration;

    mutex mutex_;
    condition_variable cv_;
    int notification_received = 0;

    WatchTower::Registration registerFile( const string& file_name ) {
        weak_ptr<void> weakHeartbeat( heartbeat_ );

        auto reg = watch_tower->addFile( file_name, [this, weakHeartbeat] (void) {
            // Ensure the fixture object is still alive using the heartbeat
            if ( auto keep = weakHeartbeat.lock() ) {
                unique_lock<mutex> lock(mutex_);
                ++notification_received;
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

    bool waitNotificationReceived( int number = 1 ) {
        unique_lock<mutex> lock(mutex_);
        bool result = ( cv_.wait_for( lock, std::chrono::milliseconds(TIMEOUT),
                [this, number] { return notification_received >= number; } ) );

        // Reinit the notification
        notification_received = 0;

        return result;
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

#ifdef _WIN32
const int WatchTowerSingleFile::TIMEOUT = 2000;
#else
const int WatchTowerSingleFile::TIMEOUT = 20;
#endif

#include "log.h"

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

TEST_F( WatchTowerSingleFile, TwoWatchesOnSameFileYieldsTwoNotifications ) {
    auto second_registration = registerFile( file_name );
    appendDataToFile( file_name );

    ASSERT_TRUE( waitNotificationReceived( 2 ) );
}

TEST_F( WatchTowerSingleFile, RemovingOneWatchOfTwoStillYieldsOneNotification ) {
    {
        auto second_registration = registerFile( file_name );
    }

    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived( 1 ) );
}

TEST_F( WatchTowerSingleFile, RenamingTheFileYieldsANotification ) {
    auto new_file_name = createTempEmptyFile();
    remove( new_file_name.c_str() );

    rename( file_name.c_str(), new_file_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );

    rename( new_file_name.c_str(), file_name.c_str() );
}

TEST_F( WatchTowerSingleFile, RenamingAFileToTheWatchedNameYieldsANotification ) {
    remove( file_name.c_str() );
    waitNotificationReceived();

    auto new_file_name = createTempEmptyFile();
    appendDataToFile( new_file_name );

    rename( new_file_name.c_str(), file_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );
}

/*****/

#ifdef HAVE_SYMLINK
class WatchTowerSymlink: public WatchTowerSingleFile {
  public:
    string symlink_name;

    void SetUp() override {
        file_name = createTempEmptyFile();
        symlink_name = createTempEmptyFile();
        remove( symlink_name.c_str() );
        symlink( file_name.c_str(), symlink_name.c_str() );

        registration = registerFile( symlink_name );
    }

    void TearDown() override {
        remove( symlink_name.c_str() );
        remove( file_name.c_str() );
    }
};

TEST_F( WatchTowerSymlink, AppendingToTheSymlinkYieldsANotification ) {
    appendDataToFile( symlink_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( WatchTowerSymlink, AppendingToTheTargetYieldsANotification ) {
    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( WatchTowerSymlink, RemovingTheSymlinkYieldsANotification ) {
    remove( symlink_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( WatchTowerSymlink, RemovingTheTargetYieldsANotification ) {
    remove( file_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( WatchTowerSymlink, ReappearingSymlinkYieldsANotification ) {
    auto new_target = createTempEmptyFile();
    remove( symlink_name.c_str() );
    waitNotificationReceived();

    symlink( new_target.c_str(), symlink_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );

    remove( new_target.c_str() );
}
#endif //HAVE_SYMLINK

/*****/

TEST( WatchTowerLifetime, RegistrationCanBeDeletedWhenWeAreDead ) {
#if 0
    auto mortal_watch_tower = new INotifyWatchTower();
    auto reg = mortal_watch_tower->addFile( "/tmp/test_file", [] (void) { } );

    delete mortal_watch_tower;
    // reg will be destroyed after the watch_tower
#endif
}

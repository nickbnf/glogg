#include "gmock/gmock.h"

#include "config.h"

#include <cstdio>
#include <fcntl.h>

#include <memory>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <chrono>

#include "log.h"

#ifndef GLOGG_USES_QTFILEWATCHER

#include <QTemporaryFile>

#include "watchtower.h"

#if defined(GLOGG_SUPPORTS_WINWATCH)
#  include "winwatchtowerdriver.h"
using PlatformWatchTower = WatchTower<WinWatchTowerDriver>;
#elif defined(GLOGG_SUPPORTS_INOTIFY)
#  include "inotifywatchtowerdriver.h"
using PlatformWatchTower = WatchTower<INotifyWatchTowerDriver>;
#endif

using namespace std;
using namespace testing;

class WatchTowerBehaviour: public testing::Test {
  public:
    shared_ptr<PlatformWatchTower> watch_tower = make_shared<PlatformWatchTower>();

    std::string createTempName()
    {
        QTemporaryFile f;
        return f.fileName().toStdString();
    }

    string createTempEmptyFile( string file_name = "" ) {
        string tmp_name = file_name;

        if ( tmp_name.empty() ) {
            tmp_name = createTempName();
        }
        int fd = creat( tmp_name.c_str(), S_IRUSR | S_IWUSR );
        close( fd );

        return tmp_name;
    }

    string getNonExistingFileName() {
        return createTempName();
    }

    WatchTowerBehaviour() {
        // Default to quiet, but increase to debug
        FILELog::setReportingLevel( logERROR );
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

class DISABLED_WatchTowerSingleFile: public WatchTowerBehaviour {
  public:
    static const int TIMEOUT;

    mutex mutex_;
    condition_variable cv_;

    string file_name;
    Registration registration;

    int notification_received = 0;

    Registration registerFile( const string& filename ) {
        weak_ptr<void> weakHeartbeat( heartbeat_ );

        auto reg = watch_tower->addFile( filename, [this, weakHeartbeat] (void) {
            // Ensure the fixture object is still alive using the heartbeat
            if ( auto keep = weakHeartbeat.lock() ) {
                unique_lock<mutex> lock(mutex_);
                ++notification_received;
                cv_.notify_one();
            } } );

        return reg;
    }

    DISABLED_WatchTowerSingleFile()
        : heartbeat_( shared_ptr<void>( (void*) 0xDEADC0DE, [] (void*) {} ) )
    {
        file_name = createTempEmptyFile();
        registration = registerFile( file_name );
    }

    ~DISABLED_WatchTowerSingleFile() {
        remove( file_name.c_str() );
    }

    bool waitNotificationReceived( int number = 1, int timeout_ms = TIMEOUT ) {
        unique_lock<mutex> lock(mutex_);
        bool result = ( cv_.wait_for( lock, std::chrono::milliseconds(timeout_ms),
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

  private:
    // Heartbeat ensures the object is still alive
    shared_ptr<void> heartbeat_;
};

#ifdef _WIN32
const int DISABLED_WatchTowerSingleFile::TIMEOUT = 2000;
#else
const int DISABLED_WatchTowerSingleFile::TIMEOUT = 20;
#endif

TEST_F( DISABLED_WatchTowerSingleFile, SignalsWhenAWatchedFileIsAppended ) {
    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( DISABLED_WatchTowerSingleFile, SignalsWhenAWatchedFileIsRemoved) {
    remove( file_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( DISABLED_WatchTowerSingleFile, SignalsWhenADeletedFileReappears ) {
    remove( file_name.c_str() );
    waitNotificationReceived();
    createTempEmptyFile( file_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( DISABLED_WatchTowerSingleFile, SignalsWhenAReappearedFileIsAppended ) {
    remove( file_name.c_str() );
    waitNotificationReceived();
    createTempEmptyFile( file_name );
    waitNotificationReceived();

    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( DISABLED_WatchTowerSingleFile, StopSignalingWhenWatchDeleted ) {
    auto second_file_name = createTempEmptyFile();
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    // Ensure file creation has been 'digested'
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

TEST_F( DISABLED_WatchTowerSingleFile, SignalsWhenSameFileIsFollowedMultipleTimes ) {
    auto second_file_name = createTempEmptyFile();

    for ( int i = 0; i < 100; i++ )
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

TEST_F( DISABLED_WatchTowerSingleFile, TwoWatchesOnSameFileYieldsTwoNotifications ) {
    auto second_registration = registerFile( file_name );
    appendDataToFile( file_name );

    ASSERT_TRUE( waitNotificationReceived( 2 ) );
}

TEST_F( DISABLED_WatchTowerSingleFile, RemovingOneWatchOfTwoStillYieldsOneNotification ) {
    {
        auto second_registration = registerFile( file_name );
    }

    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived( 1 ) );
}

TEST_F( DISABLED_WatchTowerSingleFile, RenamingTheFileYieldsANotification ) {
    auto new_file_name = createTempName();

    rename( file_name.c_str(), new_file_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );

    rename( new_file_name.c_str(), file_name.c_str() );
}

TEST_F( DISABLED_WatchTowerSingleFile, RenamingAFileToTheWatchedNameYieldsANotification ) {
    remove( file_name.c_str() );
    waitNotificationReceived();

    std::string new_file_name = createTempEmptyFile();
    appendDataToFile( new_file_name );

    rename( new_file_name.c_str(), file_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );
}

/*****/

#ifdef GLOGG_SUPPORTS_SYMLINK
class DISABLED_WatchTowerSymlink: public DISABLED_WatchTowerSingleFile {
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

TEST_F( DISABLED_WatchTowerSymlink, AppendingToTheSymlinkYieldsANotification ) {
    appendDataToFile( symlink_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( DISABLED_WatchTowerSymlink, AppendingToTheTargetYieldsANotification ) {
    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( DISABLED_WatchTowerSymlink, RemovingTheSymlinkYieldsANotification ) {
    remove( symlink_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( DISABLED_WatchTowerSymlink, RemovingTheTargetYieldsANotification ) {
    remove( file_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( DISABLED_WatchTowerSymlink, ReappearingSymlinkYieldsANotification ) {
    auto new_target = createTempEmptyFile();
    remove( symlink_name.c_str() );
    waitNotificationReceived();

    symlink( new_target.c_str(), symlink_name.c_str() );
    ASSERT_TRUE( waitNotificationReceived() );

    remove( new_target.c_str() );
}

TEST_F( DISABLED_WatchTowerSymlink, DataAddedInAReappearingSymlinkYieldsANotification ) {
    auto new_target = createTempEmptyFile();
    remove( symlink_name.c_str() );
    waitNotificationReceived();
    symlink( new_target.c_str(), symlink_name.c_str() );
    waitNotificationReceived();

    appendDataToFile( new_target );
    ASSERT_TRUE( waitNotificationReceived() );

    remove( new_target.c_str() );
}
#endif //GLOGG_SUPPORTS_SYMLINK

/*****/

TEST( WatchTowerLifetime, RegistrationCanBeDeletedWhenWeAreDead ) {
    auto mortal_watch_tower = new PlatformWatchTower();
    auto reg = mortal_watch_tower->addFile( "/tmp/test_file", [] (void) { } );

    delete mortal_watch_tower;
    // reg will be destroyed after the watch_tower
}

/*****/

class DISABLED_WatchTowerDirectories: public DISABLED_WatchTowerSingleFile {
  public:
    string second_dir_name;
    string second_file_name;
    string third_file_name;

    Registration registration_two;
    Registration registration_three;

    DISABLED_WatchTowerDirectories() {
        second_dir_name = createTempDir();
        second_file_name = createTempEmptyFileInDir( second_dir_name );
        third_file_name  = createTempEmptyFileInDir( second_dir_name );
    }

    ~DISABLED_WatchTowerDirectories() {
        remove( third_file_name.c_str() );
        remove( second_file_name.c_str() );

        removeDir( second_dir_name );
    }

    string createTempDir() {
#ifdef _WIN32
        static int counter = 1;
        char temp_dir[255];

        GetTempPath( sizeof temp_dir, temp_dir );

        string dir_name = string { temp_dir } + string { "\\test" } + to_string( counter++ );
        mkdir( dir_name.c_str() );
        return dir_name;
#else
        char dir_template[] = "/tmp/XXXXXX";
        return { mkdtemp( dir_template ) };
#endif
    }

    string createTempEmptyFileInDir( const string& dir ) {
        static int counter = 1;
        return createTempEmptyFile( dir + std::string { "/temp" } + to_string( counter++ ) );
    }

    void removeDir( const string& name ) {
        rmdir( name.c_str() );
    }
};

TEST_F( DISABLED_WatchTowerDirectories, FollowThreeFilesInTwoDirs ) {
    registration_two   = registerFile( second_file_name );
    registration_three = registerFile( third_file_name );

    ASSERT_THAT( watch_tower->numberWatchedDirectories(), Eq( 2 ) );
}

TEST_F( DISABLED_WatchTowerDirectories, FollowTwoFilesInTwoDirs ) {
    registration_two   = registerFile( second_file_name );
    {
        auto temp_registration_three = registerFile( third_file_name );
    }

    ASSERT_THAT( watch_tower->numberWatchedDirectories(), Eq( 2 ) );
}

TEST_F( DISABLED_WatchTowerDirectories, FollowOneFileInOneDir ) {
    {
        auto temp_registration_two   = registerFile( second_file_name );
        auto temp_registration_three = registerFile( third_file_name );

        ASSERT_THAT( watch_tower->numberWatchedDirectories(), Eq( 2 ) );
    }

    ASSERT_THAT( watch_tower->numberWatchedDirectories(), Eq( 1 ) );
}

/*****/

class WatchTowerInexistantDirectory: public DISABLED_WatchTowerDirectories {
  public:
    WatchTowerInexistantDirectory() {
        test_dir = createTempDir();
        rmdir( test_dir.c_str() );
    }

    ~WatchTowerInexistantDirectory() {
        rmdir( test_dir.c_str() );
    }

    string test_dir;
};

TEST_F( WatchTowerInexistantDirectory, LaterCreatedDirIsFollowed ) {
    /* Dir (and file) don't exist */
    auto file_name = createTempEmptyFileInDir( test_dir );
    {
        auto registration = registerFile( file_name );

#ifdef _WIN32
        mkdir( test_dir.c_str() );
#else
        mkdir( test_dir.c_str(), 0777 );
#endif
        createTempEmptyFile( file_name );
    }

    auto registration2 = registerFile( file_name );

    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived() );

    remove( file_name.c_str() );
}

/*****/

#ifdef _WIN32
class WinNotificationInfoListTest : public testing::Test {
  public:
    using Action = WinNotificationInfo::Action;

    struct Buffer {
        uint32_t next_addr;
        uint32_t action;
        uint32_t filename_length;
        wchar_t filename[13];
    };
    static struct Buffer buffer[2];

    WinNotificationInfoList list { reinterpret_cast<char*>( buffer ), sizeof( buffer ) };
    WinNotificationInfoList::iterator iterator { std::begin( list ) };
};

struct WinNotificationInfoListTest::Buffer WinNotificationInfoListTest::buffer[] =
    { { 40, 1, 26, L"Filename.txt" },
      { 0, 3, 18, L"file2.txt" } };

TEST_F( WinNotificationInfoListTest, FirstNotificationCanBeObtained ) {
    auto notification = *iterator;
    ASSERT_THAT( &notification, NotNull() );
}

TEST_F( WinNotificationInfoListTest, FirstNotificationHasRightAction ) {
    ASSERT_THAT( (*iterator).action(), Eq( Action::ADDED ) );
}

TEST_F( WinNotificationInfoListTest, FirstNotificationHasRightFileName ) {
    ASSERT_THAT( (*iterator).fileName(), Eq( std::wstring( L"Filename.txt\0", 13 ) ) );
}

TEST_F( WinNotificationInfoListTest, SecondNotificationCanBeObtained ) {
    ++iterator;
    auto notification = *iterator;
    ASSERT_THAT( &notification, NotNull() );
}

TEST_F( WinNotificationInfoListTest, SecondNotificationIsCorrect ) {
    iterator++;
    ASSERT_THAT( iterator->action(), Eq( Action::MODIFIED ) );
    ASSERT_THAT( iterator->fileName(), Eq( L"file2.txt" ) );
}

TEST_F( WinNotificationInfoListTest, CanBeIteratedByFor ) {
    for ( auto notification : list ) {
        notification.action();
    }
}
#endif

/*****/

#ifdef _WIN32
class WatchTowerPolling : public DISABLED_WatchTowerSingleFile {
  public:
    WatchTowerPolling() : DISABLED_WatchTowerSingleFile() {
        // FILELog::setReportingLevel( logDEBUG );

        fd_ = open( file_name.c_str(), O_WRONLY | O_APPEND );
    }

    ~WatchTowerPolling() {
        close( fd_ );
    }

    void appendDataToFileWoClosing() {
        static const char* string = "Test line\n";
        write( fd_, (void*) string, strlen( string ) );
    }

    int fd_;
};

TEST_F( WatchTowerPolling, OpenFileDoesNotGenerateImmediateNotification ) {
    appendDataToFileWoClosing();
    ASSERT_FALSE( waitNotificationReceived() );
}

TEST_F( WatchTowerPolling, OpenFileYieldsAPollNotification ) {
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    watch_tower->setPollingInterval( 500 );
    appendDataToFileWoClosing();
    ASSERT_TRUE( waitNotificationReceived() );
}

TEST_F( WatchTowerPolling, UnchangedFileDoesNotYieldANotification ) {
    watch_tower->setPollingInterval( 500 );
    ASSERT_FALSE( waitNotificationReceived() );
}

TEST_F( WatchTowerPolling, FileYieldsAnImmediateNotification ) {
    watch_tower->setPollingInterval( 4000 );
    appendDataToFile( file_name );
    ASSERT_TRUE( waitNotificationReceived( 1, 2000 ) );
}

TEST_F( WatchTowerPolling, PollIsDelayedIfImmediateNotification ) {
    watch_tower->setPollingInterval( 500 );
    appendDataToFile( file_name );
    waitNotificationReceived();
    appendDataToFileWoClosing();
    std::this_thread::sleep_for( std::chrono::milliseconds( 400 ) );
    ASSERT_FALSE( waitNotificationReceived( 1, 250 ) );
    ASSERT_TRUE( waitNotificationReceived() );
}

#endif

#endif // GLOGG_USES_QTFILEWATCHER

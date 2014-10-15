#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinBase.h>

#include "winwatchtower.h"

#include "log.h"

namespace {
    std::string shortstringize( const std::wstring& long_string );
};

// Utility classes

WinNotificationInfoList::WinNotificationInfoList( const char* buffer, size_t buffer_size )
{
    pointer_ = buffer;
    next_ = updateCurrentNotification( pointer_ );
}

const char* WinNotificationInfoList::updateCurrentNotification(
        const char* new_position )
{
    using Action = WinNotificationInfo::Action;

    static const std::map<uint16_t, Action> int_to_action = {
        { FILE_ACTION_ADDED, Action::ADDED },
        { FILE_ACTION_REMOVED, Action::REMOVED },
        { FILE_ACTION_MODIFIED, Action::MODIFIED },
        { FILE_ACTION_RENAMED_OLD_NAME, Action::RENAMED_OLD_NAME },
        { FILE_ACTION_RENAMED_NEW_NAME, Action::RENAMED_NEW_NAME },
    };

    uint32_t next_offset = *( reinterpret_cast<const uint32_t*>( new_position ) );
    uint32_t action      = *( reinterpret_cast<const uint32_t*>( new_position ) + 1 );
    uint32_t length      = *( reinterpret_cast<const uint32_t*>( new_position ) + 2 );

    const std::wstring file_name = { reinterpret_cast<const wchar_t*>( new_position + 12 ), length / 2 };

    LOG(logDEBUG) << "Next: " << next_offset;
    LOG(logDEBUG) << "Action: " << action;
    LOG(logDEBUG) << "Length: " << length;

    current_notification_ = WinNotificationInfo( int_to_action.at( action ), file_name );

    return ( next_offset == 0 ) ? nullptr : new_position + next_offset;
}

const char* WinNotificationInfoList::advanceToNext()
{
    pointer_ = next_;
    if ( pointer_ )
        next_ = updateCurrentNotification( pointer_ );

    return pointer_;
}

// WinWatchTower

WinWatchTower::WinWatchTower() : WatchTower(), thread_(), hCompPort_(0)
{
    running_ = true;
    thread_ = std::thread( &WinWatchTower::run, this );
}

WinWatchTower::~WinWatchTower()
{
    running_ = false;
    PostQueuedCompletionStatus( hCompPort_, 0, 0, NULL );
    thread_.join();
}

// This implementation is blocking, i.e. it will wait until the file
// is effectively loaded in the watchtower thread.
WatchTower::Registration WinWatchTower::addFile(
        const std::string& file_name,
        std::function<void()> notification )
{
    // Add will be done in the watchtower thread
    {
        std::lock_guard<std::mutex> lk( action_mutex_ );
        scheduled_action_ = std::make_shared<Action>( [this, file_name] {
            addFile( file_name );
        } );
    }

    // Poke the thread
    PostQueuedCompletionStatus( hCompPort_, 0, 0, NULL );

    // Wait for the add task to be completed
    {
        std::unique_lock<std::mutex> lk( action_mutex_ );
        action_done_cv_.wait( lk,
                [this]{ return ( scheduled_action_ != nullptr ); } );
    }
}

// Add a file (run in the context of the WatchTower thread)
void WinWatchTower::addFile( const std::string& file_name )
{
    LOG(logDEBUG) << "Adding: " << file_name;
    auto new_file = file_list_.addNewObservedFile( ObservedFile(
                file_name,
                nullptr, 0, 0 )
            );

    LOG(logDEBUG) << "new_file = " << new_file;

    auto dir = file_list_.watchedDirectoryForFile( file_name );
    if ( ! dir )
    {
        LOG(logDEBUG) << "Adding dir for: " << file_name;
        auto dir = file_list_.addWatchedDirectoryForFile( file_name );

        LOG(logDEBUG) << "Dir is: " << dir->path;
        // Open the directory
        HANDLE hDir = CreateFile( dir->path.c_str(),
                FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                NULL);

        //create a IO completion port/or associate this key with
        //the existing IO completion port
        hCompPort_ = CreateIoCompletionPort( hDir,
                hCompPort_, //if m_hCompPort is NULL, hDir is associated with a NEW completion port,
                //if m_hCompPort is NON-NULL, hDir is associated with the existing completion port that the handle m_hCompPort references
                reinterpret_cast<DWORD_PTR>( dir.get() ), //the completion 'key' is the address of the ObservedDir
                0);

        memset( &overlapped_, 0, sizeof overlapped_ );

        bool status = ReadDirectoryChangesW( hDir,
                dir->protocolInfo()->buffer_,
                dir->protocolInfo()->buffer_length_,
                false,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                &buffer_length_,// not set when using asynchronous mechanisms...
                &overlapped_,
                NULL);          // no completion routine

        LOG(logDEBUG) << "ReadDirectoryChangesW returned " << status << " (" << GetLastError() << ")";
    }

    new_file->dir_ = dir;
}

// Run in its own thread
void WinWatchTower::run()
{
    hCompPort_ = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
            NULL,
            0x0,
            0);

    while ( running_ )
    {
        unsigned long long key = 0;
        DWORD num_bytes = 0;
        LPOVERLAPPED lpOverlapped = 0;

        LOG(logDEBUG) << "Thread";

        BOOL status = GetQueuedCompletionStatus( hCompPort_,
                &num_bytes,
                &key,
                &lpOverlapped,
                2000 );

        LOG(logDEBUG) << "One " << status << " " << key;

        if ( key ) {
            // Extract the dir from the completion key
            ObservedDir* dir = reinterpret_cast<ObservedDir*>( key );
            LOG(logDEBUG) << "Got event for dir " << dir->path;

            WinNotificationInfoList notification_info(
                    dir->protocolInfo()->buffer_,
                    dir->protocolInfo()->buffer_length_ );

            for ( auto notification : notification_info ) {
                std::string file_path = dir->path + shortstringize( notification.fileName() );
                auto file = file_list_.searchByName( file_path );

                if ( file )
                {
                    for ( auto observer: file->callbacks )
                    {
                        // Here we have to cast our generic pointer back to
                        // the function pointer in order to perform the call
                        const std::shared_ptr<std::function<void()>> fptr =
                            std::static_pointer_cast<std::function<void()>>( observer );
                        // The observer is called with the mutex held,
                        // Let's hope it doesn't do anything too funky.
                        (*fptr)();
                    }
                }
            }
        }
        else {
            LOG(logDEBUG) << "Signaled";
        }

        {
            std::lock_guard<std::mutex> lk( action_mutex_ );
            if ( scheduled_action_ ) {
                (*scheduled_action_)( hCompPort_ );
                scheduled_action_ = nullptr;
                action_done_cv_.notify_all();
            }
        }
    }
    LOG(logDEBUG) << "Closing thread";
}

#include <iostream>

namespace {
    std::string shortstringize( const std::wstring& long_string )
    {
        std::string short_result {};

        for ( wchar_t c : long_string ) {
            // FIXME: that does not work for non ASCII char!!
            char short_c = static_cast<char>( c & 0x00FF );
            short_result += short_c;
        }

        return short_result;
    }
};

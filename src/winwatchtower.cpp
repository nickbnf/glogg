#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinBase.h>

#include "winwatchtower.h"

#include "log.h"

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

        // LOG(logDEBUG) << "Dir is: " << dir->path;
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
                &buffer_[0],//<--FILE_NOTIFY_INFORMATION records are put into this buffer
                READ_DIR_CHANGE_BUFFER_SIZE,
                false,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                &buffer_length_,//this var not set when using asynchronous mechanisms...
                &overlapped_,
                NULL); //no completion routine!

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
                INFINITE);

        LOG(logDEBUG) << "One " << status << " " << key;

        if ( key ) {
            // Extract the dir from the completion key
            ObservedDir* dir = reinterpret_cast<ObservedDir*>( key );
            LOG(logDEBUG) << "Got event for dir " << dir->path;
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

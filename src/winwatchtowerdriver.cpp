#include "winwatchtowerdriver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>

#include <map>

#include "watchtowerlist.h"
#include "utils.h"
#include "log.h"

namespace {
    std::string shortstringize( const std::wstring& long_string );
    std::wstring longstringize( const std::string& short_string );
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

// WinWatchTowerDriver

WinWatchTowerDriver::WinWatchTowerDriver()
{
    hCompPort_ = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
            NULL,
            0x0,
            0);
}

WinWatchTowerDriver::~WinWatchTowerDriver()
{
}

WinWatchTowerDriver::FileId WinWatchTowerDriver::addFile(
        const std::string& file_name )
{
    // Nothing for Windows
    return { };
}

WinWatchTowerDriver::SymlinkId WinWatchTowerDriver::addSymlink(
        const std::string& file_name )
{
    // Nothing for Windows
    return { };
}

// This implementation is blocking, i.e. it will wait until the file
// is effectively loaded in the watchtower thread.
WinWatchTowerDriver::DirId WinWatchTowerDriver::addDir(
        const std::string& file_name )
{
    DirId dir_id { };

    // Add will be done in the watchtower thread
    {
        /*
        std::lock_guard<std::mutex> lk( action_mutex_ );
        scheduled_action_ = std::make_unique<Action>( [this, file_name, &dir_id] {
            serialisedAddDir( file_name, dir_id );
        } );
        */
        serialisedAddDir( file_name, dir_id );
    }

    // Poke the thread
    PostQueuedCompletionStatus( hCompPort_, 0, 0, NULL );

    // Wait for the add task to be completed
    {
        /*
        std::unique_lock<std::mutex> lk( action_mutex_ );
        action_done_cv_.wait( lk,
                [this]{ return ( scheduled_action_ == nullptr ); } );
                */
    }

    LOG(logDEBUG) << "addDir returned " << dir_id.dir_record_;

    return dir_id;
}


void WinWatchTowerDriver::removeFile(
        const WinWatchTowerDriver::FileId& )
{
}

void WinWatchTowerDriver::removeSymlink( const SymlinkId& )
{
}

void WinWatchTowerDriver::removeDir( const DirId& dir_id )
{
    LOG(logDEBUG) << "Entering driver::removeDir";
    if ( dir_id.dir_record_ ) {
        void* handle = dir_id.dir_record_->handle_;

        LOG(logDEBUG) << "WinWatchTowerDriver::removeDir handle=" << std::hex << handle;

        CloseHandle( handle );
    }
    else {
        /* Happens when an error occured when creating the dir_record_ */
    }
}

//
// Private functions
//

// Add a file (run in the context of the WatchTower thread)
void WinWatchTowerDriver::serialisedAddDir(
        const std::string& dir_name,
        DirId& dir_id )
{
    bool inserted = false;
    auto dir_record = std::make_shared<WinWatchedDirRecord>( dir_name );
    // The index we will be inserting this record (if success), plus 1 (to avoid
    // 0 which is used as a magic value)
    unsigned int index_record = dir_records_.size() + 1;

    LOG(logDEBUG) << "Adding dir for: " << dir_name;

    // Open the directory
    HANDLE hDir = CreateFile(
#ifdef UNICODE
            longstringize( dir_name ).c_str(),
#else
            ( dir_name ).c_str(),
#endif
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            NULL );

    if ( hDir == INVALID_HANDLE_VALUE ) {
        LOG(logERROR) << "CreateFile failed for dir " << dir_name;
    }
    else {
        dir_record->handle_ = hDir;

        //create a IO completion port/or associate this key with
        //the existing IO completion port
        hCompPort_ = CreateIoCompletionPort( hDir,
                hCompPort_, //if m_hCompPort is NULL, hDir is associated with a NEW completion port,
                //if m_hCompPort is NON-NULL, hDir is associated with the existing completion port that the handle m_hCompPort references
                // We use the index (plus 1) of the weak_ptr as a key
                index_record,
                0 );

        LOG(logDEBUG) << "Weak ptr address stored: " << index_record;

        memset( &overlapped_, 0, sizeof overlapped_ );

        inserted = ReadDirectoryChangesW( hDir,
                dir_record->buffer_,
                dir_record->buffer_length_,
                false,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                &buffer_length_, // not set when using asynchronous mechanisms...
                &overlapped_,
                NULL );          // no completion routine

        if ( ! inserted ) {
            LOG(logERROR) << "ReadDirectoryChangesW failed (" << GetLastError() << ")";
            CloseHandle( hDir );
        }
        else {
            dir_id.dir_record_ = dir_record;
        }
    }

    if ( inserted ) {
        dir_records_.push_back( std::weak_ptr<WinWatchedDirRecord>( dir_record ) );
    }
}

std::vector<ObservedFile<WinWatchTowerDriver>*> WinWatchTowerDriver::waitAndProcessEvents(
        ObservedFileList<WinWatchTowerDriver>* list,
        std::unique_lock<std::mutex>* lock,
        std::vector<ObservedFile<WinWatchTowerDriver>*>* /* not needed in WinWatchTowerDriver */ )
{
    std::vector<ObservedFile<WinWatchTowerDriver>*> files_to_notify { };

    ULONG_PTR key = 0;
    DWORD num_bytes = 0;
    LPOVERLAPPED lpOverlapped = 0;

    lock->unlock();
    LOG(logDEBUG) << "waitAndProcessEvents now blocking...";
    BOOL status = GetQueuedCompletionStatus( hCompPort_,
            &num_bytes,
            &key,
            &lpOverlapped,
            INFINITE );
    lock->lock();

    LOG(logDEBUG) << "Event (" << status << ") key: " << std::hex << key;

    if ( key ) {
        // Extract the dir from the completion key
        auto dir_record_ptr = dir_records_[key - 1];
        LOG(logDEBUG) << "use_count = " << dir_record_ptr.use_count();

        if ( std::shared_ptr<WinWatchedDirRecord> dir_record = dir_record_ptr.lock() )
        {
            LOG(logDEBUG) << "Got event for dir " << dir_record.get();

            WinNotificationInfoList notification_info(
                    dir_record->buffer_,
                    dir_record->buffer_length_ );

            for ( auto notification : notification_info ) {
                std::string file_path = dir_record->path_ + shortstringize( notification.fileName() );
                LOG(logDEBUG) << "File is " << file_path;
                auto file = list->searchByName( file_path );

                if ( file )
                {
                    files_to_notify.push_back( file );
                }
            }

            // Re-listen for changes
            status = ReadDirectoryChangesW(
                    dir_record->handle_,
                    dir_record->buffer_,
                    dir_record->buffer_length_,
                    false,
                    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                    &buffer_length_,// not set when using asynchronous mechanisms...
                    &overlapped_,
                    NULL );          // no completion routine
        }
        else {
            LOG(logWARNING) << "Looks like our dir_record disappeared!";
        }
    }
    else {
        LOG(logDEBUG) << "Signaled";
    }

    {
        std::lock_guard<std::mutex> lk( action_mutex_ );
        if ( scheduled_action_ ) {
            (*scheduled_action_)();
            scheduled_action_ = nullptr;
            action_done_cv_.notify_all();
        }
    }

    /*
    // Just in case someone is waiting for an action to complete
    std::lock_guard<std::mutex> lk( action_mutex_ );
    scheduled_action_ = nullptr;
    action_done_cv_.notify_all();
    */
    return files_to_notify;
}

void WinWatchTowerDriver::interruptWait()
{
    LOG(logDEBUG) << "Driver::interruptWait()";
    PostQueuedCompletionStatus( hCompPort_, 0, 0, NULL );
}

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

    std::wstring longstringize( const std::string& short_string )
    {
        std::wstring long_result {};

        for ( char c : short_string ) {
            wchar_t long_c = static_cast<wchar_t>( c );
            long_result += long_c;
        }

        return long_result;
    }
};

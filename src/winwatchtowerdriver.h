/*
 * Copyright (C) 2015 Nicolas Bonnefon and other contributors
 *
 * This file is part of glogg.
 *
 * glogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WINWATCHTOWERDRIVER_H
#define WINWATCHTOWERDRIVER_H

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iterator>
#include <vector>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Utility class

// Encapsulate a directory notification returned by Windows'
// ReadDirectoryChangesW.
class WinNotificationInfo {
  public:
    enum class Action { UNDEF,
        ADDED,
        REMOVED,
        MODIFIED,
        RENAMED_OLD_NAME,
        RENAMED_NEW_NAME };

    WinNotificationInfo() { action_ = Action::UNDEF; }
    WinNotificationInfo( Action action, std::wstring file_name )
    { action_ = action; file_name_ = file_name; }

    Action action() const { return action_; }
    std::wstring fileName() const { return file_name_; }

  private:
    Action action_;
    std::wstring file_name_;
};

class WinNotificationInfoList {
  public:
    WinNotificationInfoList( const char* buffer, size_t buffer_size );

    // Iterator
    class iterator : std::iterator<std::input_iterator_tag, WinNotificationInfo> {
      public:
        iterator( WinNotificationInfoList* list, const char* position )
        { list_ = list; position_ = position; }

        const WinNotificationInfo& operator*() const
        { return list_->current_notification_; }

        const WinNotificationInfo* operator->() const
        { return &( list_->current_notification_ ); }

        const WinNotificationInfo& operator++() {
            position_ = list_->advanceToNext();
            return list_->current_notification_;
        }

        WinNotificationInfo operator++( int ) {
            WinNotificationInfo tmp { list_->current_notification_ };
            operator++();
            return tmp;
        }

        bool operator!=( const iterator& other ) const {
            return ( list_ != other.list_ ) || ( position_ != other.position_ );
        }

      private:
        WinNotificationInfoList* list_;
        const char* position_;
    };

    iterator begin() { return iterator( this, pointer_ ); }
    iterator end() { return iterator( this, nullptr ); }

  private:
    const char* advanceToNext();

    // Current notification (in the byte stream)
    const char* pointer_;
    // Next notification (in the byte stream)
    const char* next_;
    WinNotificationInfo current_notification_;

    const char* updateCurrentNotification( const char* new_position );
};

template <typename Driver>
class ObservedFile;
template <typename Driver>
class ObservedFileList;

class WinWatchTowerDriver {
  public:
    struct WinWatchedDirRecord {
        WinWatchedDirRecord( const std::string& file_name )
            : path_( file_name ) { }

        static const int READ_DIR_CHANGE_BUFFER_SIZE = 4096;

        std::string path_;
        void* handle_ = nullptr;
        static const unsigned long buffer_length_ = READ_DIR_CHANGE_BUFFER_SIZE;
        char buffer_[buffer_length_];
    };

    class FileId { };
    class SymlinkId { };
    class DirId {
      public:
        friend class WinWatchTowerDriver;

        DirId() {}
        bool operator==( const DirId& other ) const
        { return dir_record_ == other.dir_record_; }
        bool valid() const
        { return ( dir_record_ != nullptr ); }
      private:
        std::shared_ptr<WinWatchedDirRecord> dir_record_;
    };

    // On Windows, the token is the "last write" time
    class FileChangeToken {
      public:
        FileChangeToken() {}
        FileChangeToken( const std::string& file_name )
        { readFromFile( file_name ); }

        void readFromFile( const std::string& file_name );

        bool operator==( const FileChangeToken& other )
        { return ( low_date_time_ == other.low_date_time_ ) &&
            ( high_date_time_ == other.high_date_time_ ) &&
            ( low_file_size_  == other.low_file_size_ ) &&
            ( high_file_size_ == other.high_file_size_); }
        bool operator!=( const FileChangeToken& other )
        { return ! operator==( other ); }

      private:
        DWORD low_date_time_  = 0;
        DWORD high_date_time_ = 0;
        DWORD low_file_size_  = 0;
        DWORD high_file_size_ = 0;
    };

    // Default constructor
    WinWatchTowerDriver();
    ~WinWatchTowerDriver();

    FileId addFile( const std::string& file_name );
    SymlinkId addSymlink( const std::string& file_name );
    DirId addDir( const std::string& file_name );

    void removeFile( const FileId& file_id );
    void removeSymlink( const SymlinkId& symlink_id );
    void removeDir( const DirId& dir_id );

    std::vector<ObservedFile<WinWatchTowerDriver>*> waitAndProcessEvents(
            ObservedFileList<WinWatchTowerDriver>* list,
            std::unique_lock<std::mutex>* lock,
            std::vector<ObservedFile<WinWatchTowerDriver>*>* files_needing_readding,
            int timout_ms );

    // Interrupt waitAndProcessEvents
    void interruptWait();

  private:
    // An action which will be picked up by the worker thread.
    class Action {
      public:
        Action( std::function<void()> function ) : function_ { function } {}
        ~Action() {}

        void operator()() { function_(); }

      private:
        std::function<void()> function_;
    };

    // Action
    std::mutex action_mutex_;
    std::condition_variable action_done_cv_;
    std::unique_ptr<Action> scheduled_action_ = nullptr;

    // Win32 notification variables
    HANDLE     hCompPort_;
    OVERLAPPED overlapped_;
    unsigned long buffer_length_;

    // List of directory records
    // Accessed exclusively in the worker thread context
    std::vector<std::weak_ptr<WinWatchedDirRecord>> dir_records_ { };

    // Private member functions
    void serialisedAddDir(
            const std::string& file_name,
            DirId& dir_id );
};

#endif

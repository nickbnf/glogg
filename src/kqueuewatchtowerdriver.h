/*
 * Copyright (C) 2018 Nicolas Bonnefon and other contributors
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

#ifndef KQUEUEWATCHTOWERDRIVER_H
#define KQUEUEWATCHTOWERDRIVER_H

#include <memory>
#include <mutex>
#include <vector>

template <typename Driver>
struct ObservedFile;
template <typename Driver>
class ObservedFileList;

class KQueueWatchTowerDriver {
  public:
    class FileId {
      public:
        friend class KQueueWatchTowerDriver;

        FileId() { fd_ = -1; }
        bool operator==( const FileId& other ) const
        { return fd_ == other.fd_; }
      private:
        FileId( int fd ) { fd_ = fd; }
        int fd_;
    };
    class DirId {
      public:
        friend class KQueueWatchTowerDriver;

        DirId() { wd_ = -1; }
        bool operator==( const DirId& other ) const
        { return wd_ == other.wd_; }
        bool valid() const
        { return (wd_ != -1); }
      private:
        DirId( int wd ) { wd_ = wd; }
        int wd_;
    };
    class SymlinkId {
      public:
        friend class KQueueWatchTowerDriver;

        SymlinkId() { wd_ = -1; }
        bool operator==( const SymlinkId& other ) const
        { return wd_ == other.wd_; }
      private:
        SymlinkId( int wd ) { wd_ = wd; }
        int wd_;
    };

    // Dummy class for inotify
    class FileChangeToken {
      public:
        FileChangeToken() {}
        FileChangeToken( const std::string& ) {}

        void readFromFile( const std::string& ) {}

        bool operator!=( const FileChangeToken& )
        { return true; }
    };

#ifdef HAS_TEMPLATE_ALIASES
    using KQueueObservedFile = ObservedFile<KQueueWatchTowerDriver>;
    using KQueueObservedFileList = ObservedFileList<KQueueWatchTowerDriver>;
#else
    typedef ObservedFile<KQueueWatchTowerDriver> KQueueObservedFile;
    typedef ObservedFileList<KQueueWatchTowerDriver> KQueueObservedFileList;
#endif

    // Default constructor
    KQueueWatchTowerDriver();
    ~KQueueWatchTowerDriver();

    // No copy/assign/move please
    KQueueWatchTowerDriver( const KQueueWatchTowerDriver& ) = delete;
    KQueueWatchTowerDriver& operator=( const KQueueWatchTowerDriver& ) = delete;
    KQueueWatchTowerDriver( const KQueueWatchTowerDriver&& ) = delete;
    KQueueWatchTowerDriver& operator=( const KQueueWatchTowerDriver&& ) = delete;

    FileId addFile( const std::string& file_name );
    SymlinkId addSymlink( const std::string& file_name );
    DirId addDir( const std::string& file_name );

    void removeFile( const FileId& file_id );
    void removeSymlink( const SymlinkId& symlink_id );
    void removeDir( const DirId& dir_id );

    // Wait for an event for the OS, treat it and
    // return a list of files to notify about.
    // This must be called with the lock on the list held,
    // the function will unlock it temporary whilst blocking.
    // Also returns a list of file that need readding
    // (because of renames/symlink...)
    std::vector<KQueueObservedFile*> waitAndProcessEvents(
            KQueueObservedFileList* list,
            std::unique_lock<std::mutex>* list_mutex,
            std::vector<KQueueObservedFile*>* files_needing_readding,
            int timeout_ms );

    // Interrupt waitAndProcessEvents if it is blocking.
    void interruptWait();

  private:
    // Only written at initialisation so no protection needed.
    const int kqueue_fd_;

    // Breaking pipe
    int breaking_pipe_read_fd_;
    int breaking_pipe_write_fd_;

    /*
    // Private member functions
    size_t processINotifyEvent( const struct inotify_event* event,
            KQueueObservedFileList* list,
            std::vector<KQueueObservedFile*>* files_to_notify,
            std::vector<KQueueObservedFile*>* files_needing_readding );
            */
};

#endif

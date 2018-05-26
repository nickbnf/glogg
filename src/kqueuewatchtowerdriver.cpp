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

#include "kqueuewatchtowerdriver.h"

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include "log.h"

#include "watchtowerlist.h"

KQueueWatchTowerDriver::KQueueWatchTowerDriver() : kqueue_fd_( kqueue() )
{
    int pipefd[2];

    pipe( pipefd );
    fcntl( pipefd[0], F_SETFD, O_NONBLOCK );
    fcntl( pipefd[1], F_SETFD, O_NONBLOCK );

    breaking_pipe_read_fd_  = pipefd[0];
    breaking_pipe_write_fd_ = pipefd[1];

    struct kevent changelist;
    const struct timespec no_timeout = { 0, 0 };

    EV_SET( &changelist,
            breaking_pipe_read_fd_, // ident
            EVFILT_READ, // filter
            EV_ADD, // flags
            0, // fflags
            NULL, // data
            (void*) 0 // opaque data
          );

    // Add a watch for the breaking pipe
    if ( kevent( kqueue_fd_, &changelist, 1, NULL, 0, &no_timeout ) != -1 )
        LOG(logDEBUG) << "KQueueWatchTowerDriver::KQueueWatchTowerDriver breaking pipe monitored";
    else
        LOG(logERROR) << "KQueueWatchTowerDriver::KQueueWatchTowerDriver cannot add breaking pipe";
}

KQueueWatchTowerDriver::~KQueueWatchTowerDriver()
{
    close( breaking_pipe_read_fd_ );
    close( breaking_pipe_write_fd_ );

    close( kqueue_fd_ );
}

KQueueWatchTowerDriver::FileId KQueueWatchTowerDriver::addFile(
        const std::string& file_name )
{
    struct kevent changelist;
    const struct timespec no_timeout = { 0, 0 };
    int fd = open( file_name.c_str(), O_EVTONLY );

    if ( fd != -1 ) {
        LOG(logDEBUG) << "KQueueWatchTowerDriver::addFile new fd " << fd << " for file " << file_name;
        EV_SET( &changelist,
                fd, // ident
                EVFILT_VNODE, // filter
                EV_ADD | EV_ENABLE | EV_CLEAR, // flags
                NOTE_DELETE | NOTE_WRITE | NOTE_RENAME | NOTE_REVOKE, // fflags
                NULL, // data
                (void*) 0 // opaque data
              );

        // Add a watch for the inode
        if ( kevent( kqueue_fd_, &changelist, 1, NULL, 0, &no_timeout ) != -1 )
            LOG(logDEBUG) << "KQueueWatchTowerDriver::addFile new kevent added for fd " << fd;
        else
            LOG(logERROR) << "KQueueWatchTowerDriver::addFile cannot add file";
    }
    else {
        LOG(logERROR) << "KQueueWatchTowerDriver::addFile cannot open " << file_name;
    }

    return { fd };
}

KQueueWatchTowerDriver::SymlinkId KQueueWatchTowerDriver::addSymlink(
        const std::string& file_name )
{
    /*
    int symlink_wd = inotify_add_watch( inotify_fd_, file_name.c_str(),
            IN_DONT_FOLLOW | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF );
    LOG(logDEBUG) << "INotifyWatchTower::addFile new inotify symlink_wd " << symlink_wd;
    // (not sure a symlink can be modified but you never know)

    return { symlink_wd };
    */
    return { 0 };
}

KQueueWatchTowerDriver::DirId KQueueWatchTowerDriver::addDir(
        const std::string& file_name )
{
    /* 
    int dir_wd = inotify_add_watch( inotify_fd_, file_name.c_str(),
            IN_CREATE | IN_MOVE | IN_ONLYDIR );
    LOG(logDEBUG) << "INotifyWatchTower::addFile dir " << file_name
        << " watched wd " << dir_wd;

    return { dir_wd };
        */
    struct kevent changelist;
    const struct timespec no_timeout = { 0, 0 };
    int fd = open( file_name.c_str(), O_EVTONLY );

    if ( fd != -1 ) {
        LOG(logDEBUG) << "KQueueWatchTowerDriver::addDir new fd " << fd << " for dir " << file_name;
        EV_SET( &changelist,
                fd, // ident
                EVFILT_VNODE, // filter
                EV_ADD | EV_ENABLE | EV_CLEAR, // flags
                NOTE_WRITE, // fflags
                NULL, // data
                (void*) 0 // opaque data
              );

        // Add a watch for the inode
        if ( kevent( kqueue_fd_, &changelist, 1, NULL, 0, &no_timeout ) != -1 )
            LOG(logDEBUG) << "KQueueWatchTowerDriver::addDir new kevent added for fd " << fd;
        else
            LOG(logERROR) << "KQueueWatchTowerDriver::addDir cannot add file";
    }
    else {
        LOG(logERROR) << "KQueueWatchTowerDriver::addDir cannot open " << file_name;
    }

    return { fd };
}

void KQueueWatchTowerDriver::removeFile(
        const KQueueWatchTowerDriver::FileId& file_id )
{
    LOG(logDEBUG) << "KQueueWatchTowerDriver::removeFile removing fd " << file_id.fd_;
    close( file_id.fd_ );
}

void KQueueWatchTowerDriver::removeSymlink( const SymlinkId& symlink_id )
{
    /*
    if ( symlink_id.wd_ >= 0 )
        inotify_rm_watch( inotify_fd_, symlink_id.wd_ );
        */
}

void KQueueWatchTowerDriver::removeDir( const DirId& dir_id )
{
    /*
   LOG(logDEBUG) << "INotifyWatchTower::removeDir removing inotify wd " << dir_id.wd_;

    if ( dir_id.wd_ >= 0 )
        inotify_rm_watch( inotify_fd_, dir_id.wd_ );
        */
}

std::vector<KQueueWatchTowerDriver::KQueueObservedFile*>
KQueueWatchTowerDriver::waitAndProcessEvents(
        KQueueObservedFileList* list,
        std::unique_lock<std::mutex>* list_lock,
        std::vector<KQueueObservedFile*>* files_needing_readding,
        int timeout_ms )
{
    std::vector<KQueueObservedFile*> files_to_notify;
    struct kevent event;
    struct timespec timeout = { timeout_ms / 1000, (timeout_ms % 1000) * 1000000};

    list_lock->unlock();
    int event_count = kevent( kqueue_fd_, NULL, 0, &event, 1, timeout_ms ? &timeout : NULL );
    list_lock->lock();

    LOG(logDEBUG) << "KQueueWatchTowerDriver::waitAndProcessEvents: event_count " << event_count;

    if ( event.flags == EV_ERROR )
    {
        LOG(logERROR) << "kqueue event error!";
    }
    else if ( event_count > 0 )
    {
        LOG(logDEBUG) << "kqueue event received: " << event.ident << " " <<
            std::hex << event.filter << " " << event.flags << " " << event.fflags;

        KQueueObservedFile* file = nullptr;

        // Retrieve the file (if it was file)
        file = list->searchByFileOrSymlinkWd( event.ident, event.ident );

        if ( file )
        {
            LOG(logDEBUG) << "Adding file to notify list: " << std::hex << file;
            files_to_notify.push_back( file );
        }
        else
        {
            /* We signal all watched files in this directory, yes it is wasteful,
             * but kqueue doesn't give us enough information so we reach for
             * the sledgehammer rather than trying to see what has changed in this dir!
             */
            LOG(logDEBUG) << "File not found, is it a directory?";
            auto files_in_dir = list->searchByDirWd( event.ident );
            LOG(logDEBUG) << files_in_dir.size() << " files in dir.";
            files_to_notify.reserve( files_in_dir.size() );
            files_to_notify.insert( files_to_notify.end(), files_in_dir.begin(), files_in_dir.end() );
        }
    }
    else
    {
        LOG(logDEBUG) << "No kevent...";
    }

    return files_to_notify;
}

void KQueueWatchTowerDriver::interruptWait()
{
    char byte = 'X';

    (void) write( breaking_pipe_write_fd_, (void*) &byte, sizeof byte );
}

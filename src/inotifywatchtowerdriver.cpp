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

#include "inotifywatchtowerdriver.h"

#include <sys/inotify.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#include "log.h"

#include "watchtowerlist.h"

INotifyWatchTowerDriver::INotifyWatchTowerDriver() : inotify_fd_( inotify_init() )
{
    int pipefd[2];

    pipe2( pipefd, O_NONBLOCK );

    breaking_pipe_read_fd_  = pipefd[0];
    breaking_pipe_write_fd_ = pipefd[1];
}

INotifyWatchTowerDriver::~INotifyWatchTowerDriver()
{
    close( breaking_pipe_read_fd_ );
    close( breaking_pipe_write_fd_ );
}

INotifyWatchTowerDriver::FileId INotifyWatchTowerDriver::addFile(
        const std::string& file_name )
{
    // Add a watch for the inode
    int wd = inotify_add_watch( inotify_fd_, file_name.c_str(),
            IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF );

    LOG(logDEBUG) << "INotifyWatchTower::addFile new inotify wd " << wd;

    return { wd };
}

INotifyWatchTowerDriver::SymlinkId INotifyWatchTowerDriver::addSymlink(
        const std::string& file_name )
{
    int symlink_wd = inotify_add_watch( inotify_fd_, file_name.c_str(),
            IN_DONT_FOLLOW | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF );
    LOG(logDEBUG) << "INotifyWatchTower::addFile new inotify symlink_wd " << symlink_wd;
    // (not sure a symlink can be modified but you never know)

    return { symlink_wd };
}

INotifyWatchTowerDriver::DirId INotifyWatchTowerDriver::addDir(
        const std::string& file_name )
{
    int dir_wd = inotify_add_watch( inotify_fd_, file_name.c_str(),
            IN_CREATE | IN_MOVE | IN_ONLYDIR );
    LOG(logDEBUG) << "INotifyWatchTower::addFile dir " << file_name
        << " watched wd " << dir_wd;

    return { dir_wd };
}

void INotifyWatchTowerDriver::removeFile(
        const INotifyWatchTowerDriver::FileId& file_id )
{
    /*
       LOG(logDEBUG) << "INotifyWatchTower::removeNotification removing inotify wd "
       << file->file_wd_ << " symlink_wd " << file->symlink_wd_;
       */
    if ( file_id.wd_ >= 0 )
        inotify_rm_watch( inotify_fd_, file_id.wd_ );
}

void INotifyWatchTowerDriver::removeSymlink( const SymlinkId& symlink_id )
{
    if ( symlink_id.wd_ >= 0 )
        inotify_rm_watch( inotify_fd_, symlink_id.wd_ );
}

void INotifyWatchTowerDriver::removeDir( const DirId& dir_id )
{
   LOG(logDEBUG) << "INotifyWatchTower::removeDir removing inotify wd " << dir_id.wd_;

    if ( dir_id.wd_ >= 0 )
        inotify_rm_watch( inotify_fd_, dir_id.wd_ );
}

static constexpr size_t INOTIFY_BUFFER_SIZE = 4096;

std::vector<INotifyWatchTowerDriver::INotifyObservedFile*>
INotifyWatchTowerDriver::waitAndProcessEvents(
        INotifyObservedFileList* list,
        std::unique_lock<std::mutex>* list_lock,
        std::vector<INotifyObservedFile*>* files_needing_readding,
        int timeout_ms )
{
    std::vector<INotifyObservedFile*> files_to_notify;
    struct pollfd fds[2];

    fds[0].fd      = inotify_fd_;
    fds[0].events  = POLLIN;
    fds[0].revents = 0;

    fds[1].fd      = breaking_pipe_read_fd_;
    fds[1].events  = POLLIN;
    fds[1].revents = 0;

    list_lock->unlock();
    int poll_ret = poll( fds, 2, timeout_ms ? timeout_ms : -1 );
    list_lock->lock();

    if ( poll_ret > 0 )
    {
        if ( fds[0].revents & POLLIN )
        {
            LOG(logDEBUG) << "Pollin for inotify";
            char buffer[ INOTIFY_BUFFER_SIZE ]
                __attribute__ ((aligned(__alignof__(struct inotify_event))));

            ssize_t nb = read( inotify_fd_, buffer, sizeof( buffer ) );
            LOG(logDEBUG) << "Read " << nb << " bytes";
            if ( nb > 0 )
            {
                ssize_t offset = 0;
                while ( offset < nb ) {
                    const inotify_event* event =
                        reinterpret_cast<const inotify_event*>( buffer + offset );

                    offset += processINotifyEvent( event, list,
                            &files_to_notify, files_needing_readding );
                }
            }
            else
            {
                LOG(logWARNING) << "Error reading from inotify " << errno;
            }
        }

        if ( fds[1].revents & POLLIN )
        {
            uint8_t byte;
            read( breaking_pipe_read_fd_, &byte, sizeof byte );
        }
    }

    return files_to_notify;
}

// Treats the passed event and returns the number of bytes used
size_t INotifyWatchTowerDriver::processINotifyEvent(
        const struct inotify_event* event,
        INotifyObservedFileList* list,
        std::vector<INotifyObservedFile*>* files_to_notify,
        std::vector<INotifyObservedFile*>* files_needing_readding )
{
    LOG(logDEBUG) << "Event received: 0x" << std::hex << event->mask;

    INotifyObservedFile* file = nullptr;

    if ( event->mask & ( IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF ) )
    {
        LOG(logDEBUG) << "IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF for wd " << event->wd;

        // Retrieve the file
        file = list->searchByFileOrSymlinkWd(
                { event->wd }, { event->wd } );
    }
    else if ( event->mask & ( IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM ) )
    {
        LOG(logDEBUG) << "IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM for wd " << event->wd
            << " name: " << event->name;

        // Retrieve the file
        file = list->searchByDirWdAndName( { event->wd }, event->name );

        if ( file )
        {
            LOG(logDEBUG) << "Dir change for watched file " << event->name;
            files_needing_readding->push_back( file );
        }
    }
    else
    {
        LOG(logDEBUG) << "Unexpected event: " << event->mask << " wd " << event->wd;
    }

    if ( file )
    {
        LOG(logDEBUG) << "Adding file: " << std::hex << file;
        files_to_notify->push_back( file );
    }

    return sizeof( struct inotify_event ) + event->len;
}

void INotifyWatchTowerDriver::interruptWait()
{
    char byte = 'X';

    (void) write( breaking_pipe_write_fd_, (void*) &byte, sizeof byte );
}

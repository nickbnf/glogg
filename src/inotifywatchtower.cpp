/*
 * Copyright (C) 2014 Nicolas Bonnefon and other contributors
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <limits.h>
#include <poll.h>
#include <unistd.h>
#include <algorithm>

#include "log.h"

#include "inotifywatchtower.h"

static const size_t INOTIFY_BUFFER_SIZE = 4096;

namespace {
    bool isSymLink( const std::string& file_name );
    std::string directory_path( const std::string& path );
};

INotifyWatchTower::INotifyWatchTower() : WatchTower(), thread_(), inotify_fd( inotify_init() ),
    heartBeat_(std::shared_ptr<void>((void*) 0xDEADC0DE, [] (void*) {}))
{
    running_ = true;
    thread_ = std::thread( &INotifyWatchTower::run, this );
}

INotifyWatchTower::~INotifyWatchTower()
{
    running_ = false;
    thread_.join();
}

WatchTower::Registration INotifyWatchTower::addFile(
        const std::string& file_name,
        std::function<void()> notification )
{
    LOG(logDEBUG) << "INotifyWatchTower::addFile " << file_name;

    std::lock_guard<std::mutex> lock( observers_mutex_ );

    ObservedFile* existing_observed_file =
        observed_file_list_.searchByName( file_name );

    std::shared_ptr<std::function<void()>> ptr( new std::function<void()>(std::move( notification ) ) );

    if ( ! existing_observed_file )
    {
        int symlink_wd = -1;

        // Add a watch for the inode
        int wd = inotify_add_watch( inotify_fd, file_name.c_str(),
                IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF );

        LOG(logDEBUG) << "INotifyWatchTower::addFile new inotify wd " << wd;

        if ( isSymLink( file_name ) )
        {
            // We want to follow the name (as opposed to the inode)
            // so we watch the symlink as well.
            symlink_wd = inotify_add_watch( inotify_fd, file_name.c_str(),
                    IN_DONT_FOLLOW | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF );
            LOG(logDEBUG) << "INotifyWatchTower::addFile new inotify symlink_wd " << symlink_wd;
            // (not sure a symlink can be modified but you never know)
        }

        auto new_file = observed_file_list_.addNewObservedFile(
                ObservedFile( file_name, ptr, wd, symlink_wd ) );

        auto dir = observed_file_list_.watchedDirectoryForFile( file_name );
        if ( ! dir )
        {
            LOG(logDEBUG) << "INotifyWatchTower::addFile dir for " << file_name
                << " not watched, adding...";
            dir = observed_file_list_.addWatchedDirectoryForFile( file_name );

            dir->dir_wd_ = inotify_add_watch( inotify_fd, dir->path.c_str(),
                    IN_CREATE | IN_MOVE | IN_ONLYDIR );

            LOG(logDEBUG) << "INotifyWatchTower::addFile dir " << dir->path
                << " watched wd " << dir->dir_wd_;
        }

        new_file->dir_ = dir;
    }
    else
    {
        existing_observed_file->addCallback( ptr );
    }

    std::weak_ptr<void> weakHeartBeat(heartBeat_);

    // Returns a shared pointer that removes its own entry
    // from the list of watched stuff when it goes out of scope!
    // Uses a custom deleter to do the work.
    return std::shared_ptr<void>( 0x0, [this, ptr, weakHeartBeat] (void*) {
            if ( auto heart_beat = weakHeartBeat.lock() )
                removeNotification( this, ptr );
            } );
}

//
// Private functions
//

// Called by the dtor for a registration object
void INotifyWatchTower::removeNotification(
        INotifyWatchTower* watch_tower, std::shared_ptr<void> notification )
{
    LOG(logDEBUG) << "INotifyWatchTower::removeNotification";

    std::lock_guard<std::mutex> lock( watch_tower->observers_mutex_ );

    auto file =
        watch_tower->observed_file_list_.removeCallback( notification );

    if ( file )
    {
        LOG(logDEBUG) << "INotifyWatchTower::removeNotification removing inotify wd "
            << file->file_wd_ << " symlink_wd " << file->symlink_wd_;
        if ( file->file_wd_ >= 0 )
            inotify_rm_watch( watch_tower->inotify_fd, file->file_wd_ );
        if ( file->symlink_wd_ >= 0 )
            inotify_rm_watch( watch_tower->inotify_fd, file->symlink_wd_ );
    }
}

// Run in its own thread
void INotifyWatchTower::run()
{
    struct pollfd fds[1];

    fds[0].fd     = inotify_fd;
    fds[0].events = POLLIN;

    while ( running_ )
    {
        fds[0].revents = 0;
        int poll_ret = poll( fds, 1, 0 );

        if ( ( poll_ret > 0 ) && ( fds[0].revents & POLLIN ) )
        {
            LOG(logDEBUG4) << "Pollin for inotify";
            char buffer[ INOTIFY_BUFFER_SIZE ]
                __attribute__ ((aligned(__alignof__(struct inotify_event))));

            ssize_t nb = read( inotify_fd, buffer, sizeof( buffer ) );
            if ( nb > 0 )
            {
                size_t offset = 0;
                while ( offset < nb ) {
                    const inotify_event* event =
                        reinterpret_cast<const inotify_event*>( buffer + offset );

                    offset += processINotifyEvent( event );
                }
            }
            else
            {
                LOG(logWARNING) << "Error reading from inotify " << errno;
            }
        }
    }
}

// Treats the passed event and returns the number of bytes used
size_t INotifyWatchTower::processINotifyEvent( const struct inotify_event* event )
{
    LOG(logDEBUG) << "Event received: " << std::hex << event->mask;

    std::unique_lock<std::mutex> lock( observers_mutex_ );

    ObservedFile* file = nullptr;

    if ( event->mask & ( IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF ) )
    {
        LOG(logDEBUG) << "IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF for wd " << event->wd;

        // Retrieve the file
        file = observed_file_list_.searchByFileOrSymlinkWd( event->wd );
    }
    else if ( event->mask & ( IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM ) )
    {
        LOG(logDEBUG) << "IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM for wd " << event->wd
            << " name: " << event->name;

        // Retrieve the file
        file = observed_file_list_.searchByDirWdAndName( event->wd, event->name );

        if ( file )
        {
            LOG(logDEBUG) << "Dir change for watched file " << event->name;
        }
    }
    else
    {
        LOG(logDEBUG) << "Unexpected event: " << event->mask << " wd " << event->wd;
    }

    // Call all our observers
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

    return sizeof( struct inotify_event ) + event->len;
}

namespace {
    bool isSymLink( const std::string& file_name )
    {
        struct stat buf;

        lstat( file_name.c_str(), &buf );
        return ( S_ISLNK(buf.st_mode) );
    }
};

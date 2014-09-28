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

namespace {
    bool isSymLink( const std::string& file_name );
    std::string directory_path( const std::string& path );
};

// ObservedFileList class
INotifyWatchTower::ObservedFile*
    INotifyWatchTower::ObservedFileList::searchByName( const std::string& file_name )
{
    // Look for an existing observer on this file
    auto existing_observer = observed_files_.begin();
    for ( ; existing_observer != observed_files_.end(); ++existing_observer )
    {
        if ( existing_observer->file_name_ == file_name )
        {
            LOG(logDEBUG) << "Found " << file_name;
            break;
        }
    }

    if ( existing_observer != observed_files_.end() )
        return &( *existing_observer );
    else
        return nullptr;
}

INotifyWatchTower::ObservedFile*
    INotifyWatchTower::ObservedFileList::searchByFileOrSymlinkWd( int wd )
{
    auto result = find_if( observed_files_.begin(), observed_files_.end(),
            [wd] (ObservedFile file) -> bool {
                return ( wd == file.file_wd_ ) || ( wd == file.symlink_wd_ ); } );

    if ( result != observed_files_.end() )
        return &( *result );
    else
        return nullptr;
}

INotifyWatchTower::ObservedFile*
    INotifyWatchTower::ObservedFileList::searchByDirWdAndName( int wd, const char* name )
{
    auto dir = find_if( observed_dirs_.begin(), observed_dirs_.end(),
            [wd] (std::pair<std::string,std::weak_ptr<ObservedDir>> d) -> bool {
            return ( wd == std::shared_ptr<ObservedDir>(d.second)->dir_wd_ ); } );

    std::string path = dir->first + "/" + name;

    // LOG(logDEBUG) << "Testing path: " << path;

    // Looking for the path in the files we are watching
    return searchByName( path );
}

INotifyWatchTower::ObservedFile*
    INotifyWatchTower::ObservedFileList::addNewObservedFile( ObservedFile new_observed )
{
    auto new_file = observed_files_.insert( std::begin( observed_files_ ), new_observed );

    return &( *new_file );
}

std::shared_ptr<INotifyWatchTower::ObservedFile>
    INotifyWatchTower::ObservedFileList::removeCallback(
            std::shared_ptr<void> callback )
{
    std::shared_ptr<ObservedFile> returned_file = nullptr;

    for ( auto observer = begin( observed_files_ );
            observer != end( observed_files_ ); )
    {
        LOG(logDEBUG) << "Examining entry for " << observer->file_name_;

        std::vector<std::shared_ptr<void>>& callbacks = observer->callbacks;
        callbacks.erase( std::remove(
                    std::begin( callbacks ), std::end( callbacks ), callback ),
                std::end( callbacks ) );

        /* See if all notifications have been deleted for this file */
        if ( callbacks.empty() ) {
            LOG(logDEBUG) << "Empty notification list, removing the watched file";
            returned_file = std::make_shared<ObservedFile>( *observer );
            observer = observed_files_.erase( observer );
        }
        else {
            ++observer;
        }
    }

    return returned_file;
}

std::shared_ptr<INotifyWatchTower::ObservedDir>
INotifyWatchTower::ObservedFileList::watchedDirectory( std::string dir_name )
{
    std::shared_ptr<ObservedDir> dir = nullptr;

    if ( observed_dirs_.find( dir_name ) != std::end( observed_dirs_ ) )
        dir = std::shared_ptr<ObservedDir>( observed_dirs_[ dir_name ] );

    return dir;
}

std::shared_ptr<INotifyWatchTower::ObservedDir>
INotifyWatchTower::ObservedFileList::addWatchedDirectory( std::string dir_name )
{
    auto dir = std::make_shared<ObservedDir>();

    observed_dirs_[ dir_name ] = std::weak_ptr<ObservedDir>( dir );

    return dir;
}

INotifyWatchTower::INotifyWatchTower() : thread_(), inotify_fd( inotify_init() )
{
    running_ = true;
    thread_ = std::thread( &INotifyWatchTower::run, this );
}

INotifyWatchTower::~INotifyWatchTower()
{
    running_ = false;
    thread_.join();
}

INotifyWatchTower::Registration INotifyWatchTower::addFile(
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

        std::string dir_name = directory_path( file_name );

        auto dir = observed_file_list_.watchedDirectory( dir_name );

        if ( ! dir )
        {
            LOG(logDEBUG) << "INotifyWatchTower::addFile dir " << dir_name
                << " not watched, adding...";
            dir = observed_file_list_.addWatchedDirectory( dir_name );

            dir->dir_wd_ = inotify_add_watch( inotify_fd, dir_name.c_str(),
                    IN_CREATE );

            LOG(logDEBUG) << "INotifyWatchTower::addFile dir watched wd " << dir->dir_wd_;
        }

        LOG(logDEBUG) << "INotifyWatchTower::addFile dir " << dir;
        new_file->dir_ = dir;
    }
    else
    {
        existing_observed_file->addCallback( ptr );
    }


    // Returns a shared pointer that removes its own entry
    // from the list of watched stuff when it goes out of scope!
    // Uses a custom deleter to do the work.
    return std::shared_ptr<void>( 0x0, [this, ptr] (void*) {
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
            char buffer[ sizeof( inotify_event ) + NAME_MAX + 1 ];
            struct inotify_event* event = reinterpret_cast<inotify_event*>( buffer );

            int nb = read( inotify_fd, buffer, sizeof buffer );
            if ( nb > 0 )
            {
                std::lock_guard<std::mutex> lock( observers_mutex_ );

                ObservedFile* file = nullptr;

                if ( event->mask & ( IN_MODIFY | IN_DELETE_SELF ) )
                {
                    LOG(logDEBUG) << "IN_MODIFY | IN_DELETE_SELF for wd " << event->wd;

                    // Retrieve the file
                    file = observed_file_list_.searchByFileOrSymlinkWd( event->wd );
                }
                else if ( event->mask & IN_CREATE )
                {
                    LOG(logDEBUG4) << "IN_CREATE for wd " << event->wd << " name: " << event->name;

                    // Retrieve the file
                    file = observed_file_list_.searchByDirWdAndName( event->wd, event->name );

                    if ( file )
                    {
                        LOG(logDEBUG) << "Creation of watched file " << event->name;
                    }
                }
                else
                {
                    LOG(logDEBUG) << "Unexpected event: " << event->mask;
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
            }
            else
            {
                LOG(logWARNING) << "Dodgy data received from inotify, throwing away...";
            }
        }
    }
}

namespace {
    bool isSymLink( const std::string& file_name )
    {
        struct stat buf;

        lstat( file_name.c_str(), &buf );
        return ( S_ISLNK(buf.st_mode) );
    }

    std::string directory_path( const std::string& path )
    {
        size_t slash_pos = path.rfind( "/" );

        return std::string( path, 0, slash_pos );
    }
};

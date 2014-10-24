#include "inotifywatchtowerdriver.h"

INotifyWatchTowerDriver::INotifyWatchTowerDriver() :
    WatchTowerDriver(), inotify_fd( inotify_init() )
{
}

INotifyWatchTowerDriver::addFile( const std::string& file_name )
{
    // Add a watch for the inode
    int wd = inotify_add_watch( inotify_fd, file_name.c_str(),
            IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF );

    LOG(logDEBUG) << "INotifyWatchTower::addFile new inotify wd " << wd;
}

INotifyWatchTowerDriver::addSymlink( const std::string& file_name )
{
    symlink_wd = inotify_add_watch( inotify_fd, file_name.c_str(),
            IN_DONT_FOLLOW | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF );
    LOG(logDEBUG) << "INotifyWatchTower::addFile new inotify symlink_wd " << symlink_wd;
    // (not sure a symlink can be modified but you never know)
}
INotifyWatchTowerDriver::addDir( const std::string& file_name )
{
    inotify_add_watch( inotify_fd, dir->path.c_str(),
            IN_CREATE | IN_MOVE | IN_ONLYDIR );
    LOG(logDEBUG) << "INotifyWatchTower::addFile dir " << dir->path
        << " watched wd " << dir->dir_wd_;
}

INotifyWatchTowerDriver::removeFile( const INotifyFile& file_id )
{
    /*
       LOG(logDEBUG) << "INotifyWatchTower::removeNotification removing inotify wd "
       << file->file_wd_ << " symlink_wd " << file->symlink_wd_;
       */
    if ( file->file_wd_ >= 0 )
        inotify_rm_watch( watch_tower->inotify_fd, file->file_wd_ );
}

INotifyWatchTowerDriver::removeSymlink( const INotifySymlinkId& symlink_id )
{
    if ( file->symlink_wd_ >= 0 )
        inotify_rm_watch( watch_tower->inotify_fd, file->symlink_wd_ );
}

std::vector<ObservedFile*> waitAndProcessEvents( std::shared<> list, std::mutex list_mutex )
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

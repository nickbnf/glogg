#include "watchtowerlist.h"

#include "log.h"

namespace {
    std::string directory_path( const std::string& path );
};

// ObservedFileList class
ObservedFile* ObservedFileList::searchByName( const std::string& file_name )
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

ObservedFile* ObservedFileList::searchByFileOrSymlinkWd( int wd )
{
    auto result = find_if( observed_files_.begin(), observed_files_.end(),
            [wd] (ObservedFile file) -> bool {
                return ( wd == file.file_wd_ ) || ( wd == file.symlink_wd_ ); } );

    if ( result != observed_files_.end() )
        return &( *result );
    else
        return nullptr;
}

ObservedFile* ObservedFileList::searchByDirWdAndName( int wd, const char* name )
{
    auto dir = find_if( observed_dirs_.begin(), observed_dirs_.end(),
            [wd] (std::pair<std::string,std::weak_ptr<ObservedDir>> d) -> bool {
            if ( auto dir = d.second.lock() ) {
                return ( wd == dir->dir_wd_ );
            }
            else {
                return false; } } );

    if ( dir != observed_dirs_.end() ) {
        std::string path = dir->first + "/" + name;

        // LOG(logDEBUG) << "Testing path: " << path;

        // Looking for the path in the files we are watching
        return searchByName( path );
    }
    else {
        return nullptr;
    }
}

ObservedFile* ObservedFileList::addNewObservedFile( ObservedFile new_observed )
{
    auto new_file = observed_files_.insert( std::begin( observed_files_ ), new_observed );

    return &( *new_file );
}

std::shared_ptr<ObservedFile> ObservedFileList::removeCallback(
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

std::shared_ptr<ObservedDir> ObservedFileList::watchedDirectory(
        const std::string& dir_name )
{
    std::shared_ptr<ObservedDir> dir = nullptr;

    if ( observed_dirs_.find( dir_name ) != std::end( observed_dirs_ ) )
        dir = observed_dirs_[ dir_name ].lock();

    return dir;
}

std::shared_ptr<ObservedDir> ObservedFileList::addWatchedDirectory(
        const std::string& dir_name )
{
    auto dir = std::make_shared<ObservedDir>( dir_name );

    observed_dirs_[ dir_name ] = std::weak_ptr<ObservedDir>( dir );

    return dir;
}

std::shared_ptr<ObservedDir> ObservedFileList::watchedDirectoryForFile(
        const std::string& file_name )
{
    return watchedDirectory( directory_path( file_name ) );
}

std::shared_ptr<ObservedDir> ObservedFileList::addWatchedDirectoryForFile(
        const std::string& file_name )
{
    return addWatchedDirectory( directory_path( file_name ) );
}

namespace {
    std::string directory_path( const std::string& path )
    {
        size_t slash_pos = path.rfind( '/' );

#ifdef _WIN32
        if ( slash_pos == std::string::npos ) {
            slash_pos = path.rfind( '\\' );
        }

        // We need to include the final slash on Windows
        ++slash_pos;
        LOG(logDEBUG) << "Pos = " << slash_pos;
#endif

        return std::string( path, 0, slash_pos );
    }
};

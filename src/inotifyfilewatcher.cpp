
#include "log.h"

#include "inotifyfilewatcher.h"

std::shared_ptr<INotifyWatchTower> INotifyFileWatcher::watch_tower_;

INotifyFileWatcher::INotifyFileWatcher() : FileWatcher()
{
    // Caution, this is NOT thread-safe or re-entrant!
    if ( !watch_tower_ )
    {
        watch_tower_ = std::make_shared<INotifyWatchTower>();
    }
}

INotifyFileWatcher::~INotifyFileWatcher()
{
}

void INotifyFileWatcher::addFile( const QString& fileName )
{
    LOG(logDEBUG) << "INotifyFileWatcher::addFile " << fileName.toStdString();

    watched_file_name_ = fileName;

    notification_ = std::make_shared<INotifyWatchTower::Registration>(
            watch_tower_->addFile( fileName.toStdString(), [this, fileName] {
                emit fileChanged( fileName ); } ) );
}

void INotifyFileWatcher::removeFile( const QString& fileName )
{
    LOG(logDEBUG) << "INotifyFileWatcher::removeFile " << fileName.toStdString();

    notification_ = nullptr;
}

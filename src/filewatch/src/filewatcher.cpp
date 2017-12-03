#include "filewatcher.h"

#include "log.h"

#include <vector>
#include <efsw/efsw.hpp>

#include <QFileInfo>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>

namespace  {
    struct WatchedDirecotry
    {
        efsw::WatchID watchId_;

        // filenames are in utf8
        std::string name_;
        std::vector<std::string> files_;
    };
}

class EfswFileWatcher : public efsw::FileWatchListener {
  public:
    explicit EfswFileWatcher(FileWatcher* parent) : parent_ { parent }
    {}

    void addFile( const QString& fullFileName )
    {
        QMutexLocker lock( &mutex_ );

        LOG(logDEBUG) << "QtFileWatcher::addFile " <<  fullFileName.toStdString();

        const QFileInfo fileInfo = QFileInfo( fullFileName );

        const auto filename = fileInfo.fileName().toStdString();
        const auto directory = fileInfo.canonicalPath().toStdString();

        const auto wasEmpty = watchedPaths_.empty();

        auto watchedDirectory = std::find_if(watchedPaths_.begin(), watchedPaths_.end(),
                                          [&directory](const auto& wd) { return wd.name_ == directory; });

        if ( watchedDirectory == watchedPaths_.end() ) {
            auto watchId = watcher_.addWatch( directory, this, false );

            if ( watchId < 0 ) {
                LOG(logWARNING) << "QtFileWatcher::addFile failed to add watch " << directory
                                << " error " << watchId;
                return;
            }

            WatchedDirecotry wd;
            wd.watchId_ = watchId;
            wd.name_ = directory;
            wd.files_.push_back( std::move( filename ) );

            watchedPaths_.push_back( std::move( wd ) );
        }
        else {
            if ( std::find( watchedDirectory->files_.begin(),
                            watchedDirectory->files_.end(),
                            filename ) != watchedDirectory->files_.end() ) {
                LOG(logDEBUG) << "QtFileWatcher: already watching " << filename << " in " << directory;
                return;
            }

            watchedDirectory->files_.push_back( std::move( filename ) );
        }

        if ( wasEmpty ) {
            watcher_.watch();
        }
    }

    void removeFile( const QString& fullFileName )
    {
        QMutexLocker lock( &mutex_ );

        LOG(logDEBUG) << "QtFileWatcher::removeFile " <<  fullFileName.toStdString();

        const QFileInfo fileInfo = QFileInfo( fullFileName );

        const auto filename = fileInfo.fileName().toStdString();
        const auto directory = fileInfo.absolutePath().toStdString();

        auto watchedDirectory = std::find_if(watchedPaths_.begin(), watchedPaths_.end(),
                                          [&directory](const auto& wd) { return wd.name_ == directory; });

        if ( watchedDirectory != watchedPaths_.end() ) {

            auto watchedFile = std::find( watchedDirectory->files_.begin(),
                                          watchedDirectory->files_.end(),
                                          filename );

            if ( watchedFile !=  watchedDirectory->files_.end() ) {
                watchedDirectory->files_.erase( watchedFile );
            }

            if ( watchedDirectory->files_.empty() ) {
                watcher_.removeWatch( watchedDirectory->watchId_ );
                watchedPaths_.erase( watchedDirectory );
            }
        }
        else {
            LOG(logWARNING) << "QtFileWatcher::removeFile - The file is not watched!";
        }

        for ( const auto& d : watcher_.directories() ) {
            LOG(logERROR) << "Directories still watched: " << d;
        }
    }

    void handleFileAction( efsw::WatchID watchid,
                          const std::string& dir,
                          const std::string& filename,
                          efsw::Action action,
                          std::string oldFilename ) override
    {
        Q_UNUSED( watchid );
        Q_UNUSED( action );

        auto qtDir = QString::fromStdString( dir );
        if ( qtDir.endsWith( QDir::separator() ) ) {
            qtDir.chop( 1 );
        }

        const auto directory = qtDir.toStdString();

        LOG(logDEBUG) << "QtFileWatcher::fileChangedOnDisk " << directory << " " << filename
                      << ", old name " << oldFilename;

        auto watchedDirectory = std::find_if(watchedPaths_.begin(), watchedPaths_.end(),
                                          [&directory](const auto& wd) { return wd.name_ == directory; });

        if ( watchedDirectory != watchedPaths_.end() ) {
            if ( std::find( watchedDirectory->files_.begin(),
                            watchedDirectory->files_.end(),
                            filename ) != watchedDirectory->files_.end() ) {

                LOG(logINFO) << "QtFileWatcher::fileChangedOnDisk - will notify";

                const auto changedFileName = QDir::cleanPath( QString::fromStdString( directory )
                                                              + QDir::separator()
                                                              + QString::fromStdString( filename ) );

                QMetaObject::invokeMethod(parent_, "fileChangedOnDisk", Qt::QueuedConnection, Q_ARG( QString, changedFileName ) );
            }
            else {
                LOG(logWARNING) << "QtFileWatcher::fileChangedOnDisk - call but no file monitored";
            }
        }
        else {
            LOG(logWARNING) << "QtFileWatcher::fileChangedOnDisk - call but no dir monitored";
        }
    }

  private:
    efsw::FileWatcher watcher_;
    std::vector<WatchedDirecotry> watchedPaths_;
    FileWatcher* parent_;

    QMutex mutex_;
};

void EfswFileWatcherDeleter::operator ()(EfswFileWatcher* watcher) const
{
    delete watcher;
}

FileWatcher::FileWatcher()
{
    efswWatcher_.reset( new EfswFileWatcher( this ));
}

FileWatcher::~FileWatcher() {}

FileWatcher& FileWatcher::getFileWatcher()
{
    static auto * const instance = new FileWatcher;
    return *instance;
}

void FileWatcher::addFile( const QString &fileName )
{
    efswWatcher_->addFile( fileName );
}

void FileWatcher::removeFile( const QString &fileName )
{
    efswWatcher_->removeFile( fileName );
}

void FileWatcher::fileChangedOnDisk( const QString &fileName )
{
    emit fileChanged( fileName );
}



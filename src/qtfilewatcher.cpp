/*
 * Copyright (C) 2010 Nicolas Bonnefon and other contributors
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

#include "log.h"

#include "qtfilewatcher.h"

#include <QStringList>
#include <QFileInfo>

QtFileWatcher::QtFileWatcher() : FileWatcher(), qtFileWatcher_( this )
{
    connect( &qtFileWatcher_, SIGNAL( fileChanged( const QString& ) ),
            this, SLOT( fileChangedOnDisk( const QString& ) ),
            Qt::QueuedConnection );

    connect( &qtFileWatcher_, SIGNAL( directoryChanged( const QString& ) ),
            this, SLOT( directoryChangedOnDisk( const QString& ) ),
             Qt::QueuedConnection );
}

QtFileWatcher::~QtFileWatcher()
{
    disconnect( &qtFileWatcher_ );
}

void QtFileWatcher::addFile( const QString& fileName )
{
    LOG(logDEBUG) << "QtFileWatcher::addFile " << fileName.toStdString();

    QFileInfo fileInfo = QFileInfo( fileName );

    if (monitoredFiles_.contains(fileName)) {
          LOG(logDEBUG) << "QtFileWatcher::addFile: already monitoring " << fileName.toStdString();
          return;
    }

    qtFileWatcher_.addPath( fileInfo.path() );
    if ( fileInfo.exists() ) {
        LOG(logDEBUG) << "QtFileWatcher::addFile: file exists.";
        qtFileWatcher_.addPath( fileName );
        monitoredFiles_[fileName] = FileExists;
    }
    else {
        LOG(logDEBUG) << "QtFileWatcher::addFile: file doesn't exist.";
        monitoredFiles_[fileName] = FileRemoved;
    }
}

void QtFileWatcher::removeFile( const QString& fileName )
{
    LOG(logDEBUG) << "QtFileWatcher::removeFile " << fileName.toStdString();

    QFileInfo fileInfo = QFileInfo( fileName );

    if ( monitoredFiles_.contains( fileName ) ) {
        const MonitoringState state = monitoredFiles_[fileName];
        if ( state == FileExists) {
            qtFileWatcher_.removePath( fileName );
        }
        else {
            bool needWatchDirectory = false;
            foreach ( const QString& monitoredFile, monitoredFiles_.keys() ) {
                if ( QFileInfo( monitoredFile ).path() == fileInfo.path() ) {
                    needWatchDirectory = true;
                    break;
                }
            }
            if ( !needWatchDirectory ) {
                qtFileWatcher_.removePath( fileInfo.path() );
            }
        }
        monitoredFiles_.remove( fileName );
    }
    else {
        LOG(logWARNING) << "QtFileWatcher::removeFile - The file is not watched!";
    }

    // For debug purpose:
    foreach (QString str, qtFileWatcher_.files()) {
        LOG(logERROR) << "File still watched: " << str.toStdString();
    }
    foreach (QString str, qtFileWatcher_.directories()) {
        LOG(logERROR) << "Directories still watched: " << str.toStdString();
    }
}

//
// Slots
//

void QtFileWatcher::fileChangedOnDisk( const QString& filename )
{
    LOG(logDEBUG) << "QtFileWatcher::fileChangedOnDisk " << filename.toStdString();

    if ( monitoredFiles_.contains( filename ) && monitoredFiles_[filename] == FileExists ) {
        emit fileChanged( filename );

        // If the file has been removed...
        if ( !QFileInfo( filename ).exists() )
            monitoredFiles_[filename] = FileRemoved;
    }
    else
        LOG(logWARNING) << "QtFileWatcher::fileChangedOnDisk - call from Qt but no file monitored";
}

void QtFileWatcher::directoryChangedOnDisk( const QString& filename )
{
    LOG(logDEBUG) << "QtFileWatcher::directoryChangedOnDisk " << filename.toStdString();

    foreach( const QString& monitoredFile, monitoredFiles_.keys() ) {
        const MonitoringState currentState = monitoredFiles_[monitoredFile];
        if ( currentState == FileRemoved ) {
            if ( QFileInfo( monitoredFile ).exists() ) {
                LOG(logDEBUG) << "QtFileWatcher::directoryChangedOnDisk - our file reappeared!";

                // The file has been recreated, we have to watch it again.
                monitoredFiles_[monitoredFile] = FileExists;

                // Restore the Qt file watcher (automatically cancelled
                // when the file is deleted)
                qtFileWatcher_.addPath( monitoredFile );

                emit fileChanged( monitoredFile );
            }
            else {
                LOG(logWARNING) << "QtFileWatcher::directoryChangedOnDisk - not the file we are watching";
            }
        }
        else if ( currentState == FileExists ) {
            if ( !QFileInfo( monitoredFile ).exists() ) {
                LOG(logDEBUG) << "QtFileWatcher::directoryChangedOnDisk - our file disappeared!";

                 monitoredFiles_[monitoredFile] = FileRemoved;

                emit fileChanged( monitoredFile );
            }
            else {
                LOG(logWARNING) << "QtFileWatcher::directoryChangedOnDisk - not the file we are watching";
            }
        }

    }
}

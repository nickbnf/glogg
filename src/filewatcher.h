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

#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <QObject>
#include <QFileSystemWatcher>

// This class encapsulate Qt's QFileSystemWatcher and additionally support
// watching a file that doesn't exist yet (the class will watch the owning
// directory)
// Only supports one file at the moment.
class FileWatcher : public QObject {
  Q_OBJECT

  public:
    // Create an empty object
    FileWatcher();
    // Destroy the object
    ~FileWatcher();

    // Adds the file to the list of file to watch
    // (do nothing if a file is already monitored)
    void addFile( const QString& fileName );
    // Removes the file to the list of file to watch
    // (do nothing if said file is not monitored)
    void removeFile( const QString& fileName );

  signals:
    // Sent when the file on disk has changed in any way.
    void fileChanged( const QString& );

  private slots:
    void fileChangedOnDisk( const QString& filename );
    void directoryChangedOnDisk( const QString& filename );

  private:
    enum MonitoringState { None, FileExists, FileRemoved };

    QFileSystemWatcher qtFileWatcher_;
    QString fileMonitored_;
    MonitoringState monitoringState_;
};

#endif

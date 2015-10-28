/*
 * Copyright (C) 2010, 2014 Nicolas Bonnefon and other contributors
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

// This abstract class defines a way to watch a group of (potentially
// absent) files for update.
class FileWatcher : public QObject {
  Q_OBJECT

  public:
    // Create an empty object
    FileWatcher() {}
    // Destroy the object
    virtual ~FileWatcher() {}

    // Adds the file to the list of file to watch
    // (do nothing if a file is already monitored)
    virtual void addFile( const QString& fileName ) = 0;
    // Removes the file to the list of file to watch
    // (do nothing if said file is not monitored)
    virtual void removeFile( const QString& fileName ) = 0;

    // Set the polling interval (0 means disabled)
    virtual void setPollingInterval( uint32_t ) {}

  signals:
    // Sent when the file on disk has changed in any way.
    void fileChanged( const QString& );
};

#endif

/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
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

#ifndef LOGDATA_H
#define LOGDATA_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QVector>
#include <QFileSystemWatcher>

#include "abstractlogdata.h"
#include "logfiltereddata.h"
#include "logdataworkerthread.h"

// Represents a complete set of data to be displayed (ie. a log file content)
class LogData : public AbstractLogData {
  Q_OBJECT

  public:
    // Creates an empty LogData
    LogData();
    // Destroy an object
    ~LogData();

    // Attaches (or reattaches) the LogData to a file on disk
    // It starts the asynchronous indexing and returns (almost) immediately
    // Replace the ongoing loading if necessary.
    bool attachFile( const QString& fileName );
    // Interrupt the loading and restore the previous file.
    // Does nothing if no loading in progress.
    void interruptLoading();
    // Creates a new filtered data using the passed regexp
    // ownership is passed to the caller
    LogFilteredData* getNewFilteredData() const;
    // Returns the size if the file in bytes
    qint64 getFileSize() const;

  signals:
    // Sent during the 'attach' process to signal progress
    // percent being the percentage of completion.
    void loadingProgressed( int percent );
    // Signal the client the file is fully loaded and available.
    void loadingFinished();

  private slots:
    // Consider reloading the file when it changes on disk updated
    void fileChangedOnDisk();
    void indexingFinished();

  private:
    enum MonitoredFileStatus { UNCHANGED, DATA_ADDED, TRUNCATED };
    QFileSystemWatcher fileWatcher;
    MonitoredFileStatus fileChangedOnDisk_;

    QString doGetLineString( int line ) const;
    int doGetNbLine() const;
    int doGetMaxLength() const;

    bool indexingInProgress_;
    QFile* file_;
    LinePositionArray linePosition_;
    qint64 fileSize_;
    int nbLines_;
    int maxLength_;

    LogDataWorkerThread workerThread_;
};

#endif

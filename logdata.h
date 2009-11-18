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
#include <QVarLengthArray>

#include "abstractlogdata.h"
#include "logfiltereddata.h"

// Represents a complete set of data to be displayed (ie. a log file content)
// An object of this class is immutable.
class LogData : public AbstractLogData {
  Q_OBJECT

  public:
    // Creates an empty LogData
    LogData();
    // Destroy an object
    ~LogData();

    // Attaches the LogData to a file on disk
    bool attachFile( const QString& fileName );
    // Creates a new filtered data using the passed regexp
    // ownership is passed to the caller
    LogFilteredData* getNewFilteredData( QRegExp& regExp ) const;
    // Returns the size if the file in bytes
    qint64 getFileSize() const;

  signals:
    // Sent during the 'attach' process to signal progress
    // percent being the percentage of completion.
    void loadingProgressed( int percent );

  private:
    static const int sizeChunk;

    QString doGetLineString( int line ) const;
    int doGetNbLine() const;
    int doGetMaxLength() const;

    QFile* file_;
    QVarLengthArray<qint64> linePosition_;
    qint64 fileSize_;
    int nbLines_;
    int maxLength_;
};

#endif

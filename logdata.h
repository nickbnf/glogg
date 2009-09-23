/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOGDATA_H
#define LOGDATA_H

#include <QByteArray>
#include <QString>
#include <QStringList>

#include "abstractlogdata.h"
#include "logfiltereddata.h"

// Represents a complete set of data to be displayed (ie. a log file content)
// An object of this class is immutable.
class LogData : public AbstractLogData {
  public:
    // Creates an empty LogData
    LogData();
    // Creates a log data from the data chunk passed
    LogData( const QByteArray& byteArray );

    // Creates a new filtered data using the passed regexp
    // ownership is passed to the caller
    LogFilteredData* getNewFilteredData( QRegExp& regExp ) const;

  private:
    QString doGetLineString( int line ) const;
    int doGetNbLine() const;

    QStringList* list_;
    int nbLines_;
};

#endif

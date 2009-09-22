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

#ifndef LOGFILTEREDDATA_H
#define LOGFILTEREDDATA_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QRegExp>

#include "abstractlogdata.h"

/// @brief Class encapsulating a matching line
class matchingLine {
    public:
        matchingLine( int line, QString str ) { lineNumber = line; lineString = str; };

        int lineNumber;
        QString lineString;
};

class LogFilteredData : public AbstractLogData {
    Q_OBJECT

    public:
        /// @brief Create an empty LogData
        LogFilteredData();
        LogFilteredData(QByteArray* logData, QRegExp regExp);
        LogFilteredData(QStringList* logData, QRegExp regExp);

        void runSearch();
        int getMatchingLineNumber(int lineNum) const;

    signals:
        void newDataAvailable();

    private:
        QString doGetLineString(int line) const;
        int doGetNbLine() const;

        QList<matchingLine> matchingLineList;

        QStringList* sourceLogData;
        QRegExp currentRegExp;
        bool searchDone;
};

#endif


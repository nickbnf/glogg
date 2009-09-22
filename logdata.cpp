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

#include <iostream>

#include "log.h"

#include "logdata.h"

LogData::LogData() : AbstractLogData()
{
    list = NULL;
    nbLines = 0;
}

LogData::LogData(const QByteArray &byteArray) : AbstractLogData()
{
    list = new QStringList();
    int pos=0, end=0;
    while ( (end = byteArray.indexOf("\n", pos)) != -1 ) {
        const int length = end-pos;
        const QString string = QString(byteArray.mid(pos, length));
        if ( length > maxLength)
            maxLength = length;
        pos = end+1;
        list->append(string);
    }
    nbLines = list->size();

    LOG(logDEBUG) << "Found " << nbLines << " lines.";
}

int LogData::doGetNbLine() const
{
    return nbLines;
}

QString LogData::doGetLineString(int line) const
{
    QString string = list->at(line);

    return string;
}

LogFilteredData* LogData::getNewFilteredData(QRegExp& regExp) const
{
    LogFilteredData* newFilteredData = new LogFilteredData(list, regExp);

    return newFilteredData;
}

/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

// This file implements AbstractLogData.
// This base class is primarily an interface class and should not
// implement anything.
// It exists so that AbstractLogView can manipulate an abtract set of data
// (either full or filtered).

#include "abstractlogdata.h"

AbstractLogData::AbstractLogData()
{
}

// Simple wrapper in order to use a clean Template Method
QString AbstractLogData::getLineString( qint64 line ) const
{
    return doGetLineString(line);
}

// Simple wrapper in order to use a clean Template Method
QString AbstractLogData::getExpandedLineString( qint64 line ) const
{
    return doGetExpandedLineString(line);
}

// Simple wrapper in order to use a clean Template Method
QStringList AbstractLogData::getLines( qint64 first_line, int number ) const
{
    return doGetLines( first_line, number );
}

// Simple wrapper in order to use a clean Template Method
QStringList AbstractLogData::getExpandedLines( qint64 first_line, int number ) const
{
    return doGetExpandedLines( first_line, number );
}

// Simple wrapper in order to use a clean Template Method
qint64 AbstractLogData::getNbLine() const
{
    return doGetNbLine();
}

// Simple wrapper in order to use a clean Template Method
int AbstractLogData::getMaxLength() const
{
    return doGetMaxLength();
}

// Simple wrapper in order to use a clean Template Method
int AbstractLogData::getLineLength( qint64 line ) const
{
    return doGetLineLength( line );
}

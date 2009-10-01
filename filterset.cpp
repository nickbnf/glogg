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

#include "log.h"

#include "filterset.h"

Filter::Filter( const QString& pattern,
            const QString& foreColorName, const QString& backColorName ) :
    regexp_( pattern ), foreColorName_( foreColorName ),
    backColorName_( backColorName ), enabled_( true )
{
    LOG(logDEBUG) << "New Filter, fore: " << foreColorName_.toStdString()
        << " back: " << backColorName_.toStdString();
}

QString Filter::pattern() const
{
    return regexp_.pattern();
}

void Filter::setPattern( const QString& pattern )
{
    regexp_.setPattern( pattern );
}

const QString& Filter::foreColorName() const
{
    LOG(logDEBUG) << "foreColorName(): " << foreColorName_.toStdString();
    return foreColorName_;
}

void Filter::setForeColor( const QString& foreColorName )
{
    foreColorName_ = foreColorName;
}

const QString& Filter::backColorName() const
{
    LOG(logDEBUG) << "backColorName(): " << backColorName_.toStdString();
    return backColorName_;
}

void Filter::setBackColor( const QString& backColorName )
{
    backColorName_ = backColorName;
}




FilterSet::FilterSet()
{
}

bool FilterSet::matchLine( const QString& line,
        QColor* foreColor, QColor* backColor )
{
    return false;
}


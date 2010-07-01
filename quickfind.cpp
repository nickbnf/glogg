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

// This file implements QuickFind.
// This class implements the Quick Find mechanism, it only stores the
// current search pattern, and can be asked to search the next/previous
// matching line number. Or the matches within a specific line.

#include "quickfind.h"

QuickFind::QuickFind( const AbstractLogData* log_data )
    : regexp_(), logData_( log_data )
{
    active_ = false;
}

void QuickFind::changeSearchPattern( const QString& pattern )
{
    regexp_.setPattern( pattern );

    if ( regexp_.isValid() )
        active_ = true;
}

bool QuickFind::matchLine( int line_number,
        QList<QuickFindMatch>& matches ) const
{
    if ( active_ ) {
        return false;
    }
    else {
        return false;
    }
}

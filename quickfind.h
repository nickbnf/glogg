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

#ifndef QUICKFIND_H
#define QUICKFIND_H

#include <QRegExp>

#include "abstractlogdata.h"

// Represents a match result for QuickFind
class QuickFindMatch
{
  public:
    // Construct a match (must be initialised)
    QuickFindMatch( int start_column, int length )
    { startColumn_ = start_column; length_ = length; }

    // Accessor functions
    int startColumn() const { return startColumn_; }
    int length() const { return length_; }

  private:
    int startColumn_;
    int length_;
};


// Represents a search made with Quick Find (without its results)
class QuickFind
{
  public:
    // Construct an empty search
    QuickFind( const AbstractLogData* log_data );

    // Set the search to a new pattern
    void changeSearchPattern( const QString& pattern );

    // Returns wether the passed line match the quick find search.
    // If so, it populate the passed list with the list of matches
    // within this particular line.
    bool matchLine( int line_number, QList<QuickFindMatch>& matches ) const;

  private:
    bool active_;
    QRegExp regexp_;
    const AbstractLogData* logData_;
};

#endif

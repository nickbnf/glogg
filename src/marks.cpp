/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

#include "marks.h"

#include "log.h"
#include "utils.h"

// This file implements the list of marks for a file.
// It is implemented as a QList which is kept in order when inserting,
// it is simpler than something fancy like a heap, and insertion are
// not done very often anyway.  Oh and we need to iterate through the
// list, disqualifying a straight heap.

Marks::Marks() : marks_()
{
}

void Marks::addMark( qint64 line, QChar mark )
{
    // Look for the index immediately before
    int index;
    if ( ! lookupLineNumber< QList<Mark> >( marks_, line, &index ) )
    {
        // If a mark is not already set for this line
        LOG(logDEBUG) << "Inserting mark at line " << line
            << " (index " << index << ")";
        marks_.insert( index, Mark( line ) );
    }
    else
    {
        LOG(logERROR) << "Trying to add an existing mark at line " << line;
    }

    // 'mark' is not used yet
    mark = mark;
}

qint64 Marks::getMark( QChar mark ) const
{
    // 'mark' is not used yet
    mark = mark;

    return 0;
}

bool Marks::isLineMarked( qint64 line ) const
{
    int index;
    return lookupLineNumber< QList<Mark> >( marks_, line, &index );
}

int Marks::findMark( QChar mark )
{
    // 'mark' is not used yet
    mark = mark;

    return -1;
}

void Marks::deleteMarkAt( int index )
{
    marks_.removeAt( index );
}

int Marks::deleteMark( qint64 line )
{
    int index = -1;

    if ( lookupLineNumber< QList<Mark> >( marks_, line, &index ) )
    {
        deleteMarkAt( index );
    }

    return index;
}

void Marks::clear()
{
    marks_.clear();
}

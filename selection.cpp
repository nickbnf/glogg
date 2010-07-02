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

// This file implements Selection.
// This class implements the selection handling. No check is made on
// the validity of the selection, it must be handled by the caller.
// There are three types of selection, only one type might be active
// at any time.

#include "selection.h"

Selection::Selection()
{
    selectedLine_ = -1;

    selectedPartial_.line        = -1;
    selectedPartial_.startColumn = 0;
    selectedPartial_.endColumn   = 0;
}

void Selection::selectPortion( int line, int start_column, int end_column )
{
    // First unselect any whole line
    selectedLine_ = -1;

    selectedPartial_.line = line;
    selectedPartial_.startColumn = start_column;
    selectedPartial_.endColumn = end_column;
}

bool Selection::getPortionForLine( int line, int* start_column, int* end_column ) const
{
    if ( selectedPartial_.line == line ) {
        *start_column = selectedPartial_.startColumn;
        *end_column   = selectedPartial_.endColumn;

        return true;
    }
    else {
        return false;
    }
}

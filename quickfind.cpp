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
// This class implements the Quick Find mechanism using references
// to the logData, the QFP and the selection passed.
// Search is started just after the selection and the selection is updated
// if a match is found.

#include "log.h"
#include "quickfindpattern.h"
#include "selection.h"

#include "quickfind.h"

QuickFind::QuickFind( const AbstractLogData* const logData,
        Selection* selection,
        const QuickFindPattern* const quickFindPattern ) :
    logData_( logData ), selection_( selection ), 
    quickFindPattern_( quickFindPattern )
{
}

int QuickFind::searchForward()
{
    bool found = false;
    int line;
    int column;
    int start_col;
    int end_col;

    // Position where we start the search from
    selection_->getNextPosition( &line, &column );

    // We look at the rest of the first line
    if ( quickFindPattern_->isLineMatching(
                logData_->getLineString( line ), column ) ) {
        quickFindPattern_->getLastMatch( &start_col, &end_col );
        found = true;
    }
    else {
        // And then the rest of the file
        line++;
        while ( line < logData_->getNbLine() ) {
            if ( quickFindPattern_->isLineMatching(
                        logData_->getLineString( line ) ) ) {
                quickFindPattern_->getLastMatch( &start_col, &end_col );
                found = true;
                break;
            }
            line++;
        }
    }

    if ( found )
    {
        selection_->selectPortion( line, start_col, end_col );
        return line;
    }

    return -1;
}

int QuickFind::searchBackward()
{
    bool found = false;
    int line;
    int column;
    int start_col;
    int end_col;

    // Position where we start the search from
    selection_->getPreviousPosition( &line, &column );

    LOG(logDEBUG) << "getPreviousPosition: " << line << " " << column;

    // We look at the beginning of the first line
    if ( quickFindPattern_->isLineMatchingBackward(
                logData_->getLineString( line ), column ) ) {
        quickFindPattern_->getLastMatch( &start_col, &end_col );
        found = true;
    }
    else {
        // And then the rest of the file
        line--;
        while ( line >= 0 ) {
            if ( quickFindPattern_->isLineMatching(
                        logData_->getLineString( line ) ) ) {
                quickFindPattern_->getLastMatch( &start_col, &end_col );
                found = true;
                break;
            }
            line--;
        }
    }

    if ( found )
    {
        selection_->selectPortion( line, start_col, end_col );
        return line;
    }

    return -1;
}

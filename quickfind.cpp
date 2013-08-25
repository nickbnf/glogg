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

#include <QApplication>

#include "log.h"
#include "quickfindpattern.h"
#include "selection.h"

#include "quickfind.h"

void SearchingNotifier::reset()
{
    dotToDisplay_ = 0;
    startTime_ = QTime::currentTime();
}

void SearchingNotifier::sendNotification()
{
    dotToDisplay_++;
    /*
    for ( int i=0; i < dotToDisplay_; i++ )
        message += QChar('.');
    */
    LOG( logDEBUG ) << "Emitting Searching....";
    emit notify( QFNotificationProgress( dotToDisplay_ ) );
    if ( dotToDisplay_ > 4 )
        dotToDisplay_ = 0;

    QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
    startTime_ = QTime::currentTime().addMSecs( -800 );
}

void QuickFind::LastMatchPosition::set( int line, int column )
{
    if ( ( line_ == -1 ) ||
            ( ( line <= line_ ) && ( column < column_ ) ) )
    {
        line_ = line;
        column_ = column;
    }
}

bool QuickFind::LastMatchPosition::isLater( int line, int column ) const
{
    if ( line_ == -1 )
        return false;
    else if ( ( line == line_ ) && ( column >= column_ ) )
        return true;
    else if ( line > line_ )
        return true;
    else
        return false;
}

bool QuickFind::LastMatchPosition::isSooner( int line, int column ) const
{
    if ( line_ == -1 )
        return false;
    else if ( ( line == line_ ) && ( column <= column_ ) )
        return true;
    else if ( line < line_ )
        return true;
    else
        return false;
}

QuickFind::QuickFind( const AbstractLogData* const logData,
        Selection* selection,
        const QuickFindPattern* const quickFindPattern ) :
    logData_( logData ), selection_( selection ), 
    quickFindPattern_( quickFindPattern ),
    lastMatch_(), firstMatch_(), searchingNotifier_()
{
    connect( &searchingNotifier_, SIGNAL( notify( const QFNotification& ) ),
            this, SIGNAL( notify( const QFNotification& ) ) );
}

int QuickFind::searchForward()
{
    bool found = false;
    int line;
    int column;
    int found_start_col;
    int found_end_col;

    if ( ! quickFindPattern_->isActive() )
        return -1;

    // Position where we start the search from
    selection_->getNextPosition( &line, &column );

    // Optimisation: if we are already after the last match,
    // we don't do any search at all.
    if ( lastMatch_.isLater( line, column ) ) {
        // Send a notification
        emit notify( QFNotificationReachedEndOfFile() );

        return -1;
    }

    LOG( logDEBUG ) << "Start searching.";

    // We look at the rest of the first line
    if ( quickFindPattern_->isLineMatching(
                logData_->getExpandedLineString( line ), column ) ) {
        quickFindPattern_->getLastMatch( &found_start_col, &found_end_col );
        found = true;
    }
    else {
        searchingNotifier_.reset();
        // And then the rest of the file
        line++;
        while ( line < logData_->getNbLine() ) {
            if ( quickFindPattern_->isLineMatching(
                        logData_->getExpandedLineString( line ) ) ) {
                quickFindPattern_->getLastMatch(
                        &found_start_col, &found_end_col );
                found = true;
                break;
            }
            line++;

            // See if we need to notify of the ongoing search
            searchingNotifier_.ping();
        }
    }

    if ( found ) {
        selection_->selectPortion(
                line, found_start_col, found_end_col );

        // Clear any notification
        emit clearNotification();

        return line;
    }
    else {
        // Update the position of the last match
        int last_match_line;
        int last_match_column;
        selection_->getPreviousPosition( &last_match_line, &last_match_column );
        lastMatch_.set( last_match_line, last_match_column );

        // Send a notification
        emit notify( QFNotificationReachedEndOfFile() );

        return -1;
    }
}

int QuickFind::searchBackward()
{
    bool found = false;
    int line;
    int column;
    int start_col;
    int end_col;

    if ( ! quickFindPattern_->isActive() )
        return -1;

    // Position where we start the search from
    selection_->getPreviousPosition( &line, &column );

    // Optimisation: if we are already before the first match,
    // we don't do any search at all.
    if ( firstMatch_.isSooner( line, column ) ) {
        // Send a notification
        emit notify( QFNotificationReachedBegininningOfFile() );

        return -1;
    }

    LOG( logDEBUG ) << "Start searching.";

    // We look at the beginning of the first line
    if ( ( column > 0 ) &&  ( quickFindPattern_->isLineMatchingBackward(
                logData_->getExpandedLineString( line ), column ) ) ) {
        quickFindPattern_->getLastMatch( &start_col, &end_col );
        found = true;
    }
    else {
        searchingNotifier_.reset();
        // And then the rest of the file
        line--;
        while ( line >= 0 ) {
            if ( quickFindPattern_->isLineMatchingBackward(
                        logData_->getExpandedLineString( line ) ) ) {
                quickFindPattern_->getLastMatch( &start_col, &end_col );
                found = true;
                break;
            }
            line--;

            // See if we need to notify of the ongoing search
            searchingNotifier_.ping();
        }
    }

    if ( found )
    {
        selection_->selectPortion( line, start_col, end_col );

        // Clear any notification
        emit clearNotification();

        return line;
    }
    else {
        // Update the position of the first match
        int first_match_line;
        int first_match_column;
        selection_->getNextPosition( &first_match_line, &first_match_column );
        firstMatch_.set( first_match_line, first_match_column );

        // Send a notification
        LOG( logDEBUG ) << "Send notification.";
        emit notify( QFNotificationReachedBegininningOfFile() );

        return -1;
    }
}

void QuickFind::resetLimits()
{
    lastMatch_.reset();
    firstMatch_.reset();
}

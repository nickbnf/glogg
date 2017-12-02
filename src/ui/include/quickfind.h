/*
 * Copyright (C) 2010, 2013 Nicolas Bonnefon and other contributors
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

#include <QObject>
#include <QPoint>
#include <QTime>

#include "qfnotifications.h"
#include "selection.h"

class QuickFindPattern;
class AbstractLogData;
class Portion;

// Handle "long processing" notifications to the UI.
// reset() shall be called at the beginning of the search
// and then ping() should be called periodically during the processing.
// The notify() signal should be forwarded to the UI.
class SearchingNotifier : public QObject
{
  Q_OBJECT

  public:
    SearchingNotifier() : dotToDisplay_{ 0 } {}

    // Reset internal timers at the beiginning of the processing
    void reset();
    // Shall be called frequently during processing, send the notification
    // and call the event loop when appropriate.
    // Pass the current line number and total number of line so that
    // a progress percentage is calculated and displayed.
    // (line shall be negative if ging in reverse)
    inline void ping( qint64 line, qint64 nb_lines ) {
        if ( startTime_.msecsTo( QTime::currentTime() ) > 1000 )
            sendNotification( line, nb_lines );
    }

  signals:
    // Sent when the UI shall display a message to the user.
    void notify( const QFNotification& message );

  private:
    void sendNotification( qint64 current_line, qint64 nb_lines );

    QTime startTime_;
    int dotToDisplay_;
};

// Represents a search made with Quick Find (without its results)
// it keeps a pointer to a set of data and to a QuickFindPattern which
// are used for the searches. (the caller retains ownership of both).
class QuickFind : public QObject
{
  Q_OBJECT

  public:
    // Construct a search
    QuickFind( const AbstractLogData* const logData, Selection* selection,
            const QuickFindPattern* const quickFindPattern );

    // Set the starting point that will be used by the next search
    void setSearchStartPoint( QPoint startPoint );

    // Used for incremental searches
    // Return the first occurence of the passed pattern from the starting
    // point.  These searches don't use the QFP and don't change the
    // starting point.
    // TODO Update comment
    OptionalLineNumber incrementallySearchForward();
    OptionalLineNumber incrementallySearchBackward();

    // Stop the currently ongoing incremental search, leave the selection
    // where it is if a match has been found, restore the old one
    // if not. Also throw away the start point associated with
    // the search.
    void incrementalSearchStop();

    // Throw away the current search and restore the initial
    // position/selection
    void incrementalSearchAbort();

    // Used for 'repeated' (n/N) QF searches using the current direction
    // Return the line of the first occurence of the QFP and
    // update the selection. It returns -1 if nothing is found.
    /*
    int searchNext();
    int searchPrevious();
    */

    // Idem but ignore the direction and always search in the
    // specified direction
    OptionalLineNumber searchForward();
    OptionalLineNumber searchBackward();

    // Make the object forget the 'no more match' flag.
    void resetLimits();

  signals:
    // Sent when the UI shall display a message to the user.
    void notify( const QFNotification& message );
    // Sent when the UI shall clear the notification.
    void clearNotification();

  private:
    enum QFDirection {
        None,
        Forward,
        Backward,
    };

    class LastMatchPosition {
      public:
        LastMatchPosition() : column_( -1 ) {}
        void set( LineNumber line, int column );
        void set( const FilePosition& position );
        void reset() { line_ = {}; column_ = -1; }
        // Does the passed position come after the recorded one
        bool isLater(OptionalLineNumber line, int column ) const;
        bool isLater( const FilePosition& position ) const;
        // Does the passed position come before the recorded one
        bool isSooner(OptionalLineNumber line, int column ) const;
        bool isSooner( const FilePosition& position ) const;

      private:
        OptionalLineNumber line_;
        int column_;
    };

    class IncrementalSearchStatus {
      public:
        /* Constructors */
        IncrementalSearchStatus() :
            ongoing_( None ), position_(), initialSelection_() {}
        IncrementalSearchStatus(
                QFDirection direction,
                const FilePosition& position,
                const Selection& initial_selection ) :
            ongoing_( direction ),
            position_( position ),
            initialSelection_( initial_selection ) {}

        bool isOngoing() const { return ( ongoing_ != None ); }
        QFDirection direction() const { return ongoing_; }
        FilePosition position() const { return position_; }
        Selection initialSelection() const { return initialSelection_; }
      private:
        QFDirection ongoing_;
        FilePosition position_;
        Selection initialSelection_;
    };

    // Pointers to external objects
    const AbstractLogData* const logData_;
    Selection* selection_;
    const QuickFindPattern* const quickFindPattern_;

    // Owned objects

    // Position of the last match in the file
    // (to avoid searching multiple times where there is no result)
    LastMatchPosition lastMatch_;
    LastMatchPosition firstMatch_;

    SearchingNotifier searchingNotifier_;

    // Incremental search status
    IncrementalSearchStatus incrementalSearchStatus_;

    // Private functions
    OptionalLineNumber doSearchForward( const FilePosition &start_position );
    OptionalLineNumber doSearchBackward( const FilePosition &start_position );
};

#endif

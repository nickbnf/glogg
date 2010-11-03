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

#include <QObject>
#include <QPoint>

class QuickFindPattern;
class AbstractLogData;
class Portion;
class Selection;

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
    Portion incrementallySearchForward( const QString& incPattern );
    Portion incrementallySearchBackward( const QString& incPattern );

    // Used for 'normal' (n/N) QF searches
    // Return the line of the first occurence of the QFP and
    // update the selection. It returns -1 if nothing is found.
    int searchForward();
    int searchBackward();

    // Make the object forget the 'no more match' flag.
    void resetLimits();

  signals:
    // Send when the UI shall display a message to the user.
    void notify( const QString& message );
    // Send when the UI shall clear the notification.
    void clearNotification();

  private:
    class LastMatchPosition {
      public:
        LastMatchPosition() : line_( -1 ), column_( -1 ) {}
        void set( int line, int column );
        void reset() { line_ = -1; column_ = -1; }
        // Does the passed position come after the recorded one
        bool isLater( int line, int column ) const;
        // Does the passed position come before the recorded one
        bool isSooner( int line, int column ) const;
      private:
        int line_;
        int column_;
    };

    // Pointers to external objects
    const AbstractLogData* const logData_;
    Selection* selection_;
    const QuickFindPattern* const quickFindPattern_;

    // Position of the last match in the file
    // (to avoid searching multiple times where there is no result)
    LastMatchPosition lastMatch_;
    LastMatchPosition firstMatch_;
};

#endif

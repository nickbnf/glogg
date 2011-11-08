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

#ifndef OVERVIEW_H
#define OVERVIEW_H

#include <QList>

#include "logfiltereddata.h"

// Class implementing the logic behind the matches overview bar.
// This class converts the matches found in a LogFilteredData in
// a screen dependent set of coloured lines, which is cached.
// This class is not a UI class, actual display is left to the client.
//
// This class is NOT thread-safe.
class Overview
{
  public:
    Overview( const LogFilteredData* logFilteredData );
    ~Overview();

    // Signal the overview its attached LogFilteredData has been changed and
    // the overview must be updated with the provided total number
    // of line of the file.
    void updateData( int totalNbLine );
    // Set the visibility flag of this overview.
    void setVisible( bool visible );

    // Signal the overview the height of the display has changed, triggering
    // an update of its cache.
    void updateView( int height );
    // Returns a list of lines (between 0 and 'height') representing matches.
    // (pointer returned is valid until next call to update*()
    const QList<int>* getMatchLines();
    // Returns a list of lines (between 0 and 'height') representing marks.
    // (pointer returned is valid until next call to update*()
    const QList<int>* getMarkLines();
    // Return a pair of lines (between 0 and 'height') representing the current view.
    std::pair<int,int> getViewLines( int topLine, int nbLinesOnScreen );

    // Return the line number corresponding to the passed overview y coordinate.
    int fileLineFromY( int y );

  private:
    // List of matches associated with this Overview.
    const LogFilteredData* logFilteredData_;
    // Current height of view window.
    int height_;
    // Does the cache (matchesLines, markLines) need to be recalculated.
    int dirty_;

    // List of lines representing matches and marks (are shared with the client)
    QList<int> matchLines_;
    QList<int> markLines_;
};

#endif

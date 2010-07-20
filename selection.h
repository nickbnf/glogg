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

#ifndef SELECTION_H
#define SELECTION_H

#include "abstractlogdata.h"

class Portion
{
  public:
    Portion() { line_ = -1; }
    Portion( int line, int start_column, int end_column )
    { line_ = line; startColumn_ = start_column; endColumn_ = end_column; }

    int line() const { return line_; }
    int startColumn() const { return startColumn_; }
    int endColumn() const { return endColumn_; }

    bool isValid() const { return ( line_ != -1 ); }

  private:
    int line_;
    int startColumn_;
    int endColumn_;
};

// Represents a selection in an AbstractLogView
class Selection
{
  public:
    // Construct an empty selection
    Selection();

    // Clear the selection
    void clear() { selectedPartial_.line = -1; selectedLine_ = -1; };

    // Select one line
    void selectLine( int line )
      { selectedPartial_.line = -1; selectedRange_.startLine = -1;
        selectedLine_ = line; };
    // Select a portion of line (both start and end included)
    void selectPortion( int line, int start_column, int end_column );
    void selectPortion( Portion selection )
      { selectPortion( selection.line(), selection.startColumn(),
              selection.endColumn() ); }
    // Select a range of lines (both start and end included)
    void selectRange( int start_line, int end_line );

    // Crop selection so that in fit in the range ending with the line passed.
    void crop( int last_line );

    // Returns whether the selection is a single line
    bool isSingleLine() const { return ( selectedLine_ != -1 ); }

    // Returns whether a portion is selected or not on the passed line.
    // If so, returns the portion position.
    bool getPortionForLine( int line,
            int* start_column, int* end_column ) const;
    // Get a list of selected line(s), in order.
    QList<int> getLines() const;

    // Returns wether the line passed is selected (entirely).
    bool isLineSelected( int line ) const;

    // Returns the text selected from the passed AbstractLogData
    QString getSelectedText( const AbstractLogData* logData ) const;

    // Return the position immediately after the current selection
    // (used for searches)
    void getNextPosition( int* line, int* column ) const;

  private:
    // Line number currently selected, or -1 if none selected
    int selectedLine_;

    struct SelectedPartial {
        int line;
        int startColumn;
        int endColumn;
    };
    struct SelectedRange {
        int startLine;
        int endLine;
    };
    struct SelectedPartial selectedPartial_;
    struct SelectedRange selectedRange_;
};

#endif

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

// Represents a selection in an AbstractLogView
class Selection
{
  public:
    // Construct an empty selection
    Selection();

    // Clear the selection
    void clear() { selectedLine_ = -1; };

    // Select one line
    void selectLine( int line ) { selectedLine_ = line; };
    // Select a portion of line
    void selectPortion( int line, int start_column, int end_column);
    // Select a range of lines
    void selectRange( int start_line, int end_line );

    // Crop selection so that in fit in the range ending with the line passed.
    void crop( int last_line )
    { if ( selectedLine_ > last_line ) selectedLine_ = -1; };

    // Returns wether a portion is selected or not on the passed line.
    // If so, returns the portion position.
    bool getPortionForLine( int line,
            int* start_column, int* end_column ) const;
    // Get selected whole line(s).
    int getLines() const { return selectedLine_; };

    // Returns wether the line passed is selected (entirely).
    bool isLineSelected( int line ) const { return line == selectedLine_; };

  private:
    // Line number currently selected, or -1 if none selected
    int selectedLine_;

    struct SelectedPartial {
        int line;
        int startColumn;
        int endColumn;
    };
    struct SelectedPartial selectedPartial_;
};

#endif

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

#ifndef MARKS_H
#define MARKS_H

#include <QChar>

// A list of marks, i.e. line numbers optionally associated to an
// identifying character.
class Marks {
  public:
    // Create an empty Marks
    Marks();

    // Add a mark at the given line, optionally identified by the given char
    // If a mark for this char already exist, the previous one is replaced.
    void addMark( qint64 line, QChar mark = QChar() );
    // Get the (unique) mark identified by the passed char.
    qint64 getMark( QChar mark ) const;
    // Returns wheither the passed line has a mark on it.
    bool isLineMarked( qint64 line ) const;
    // Delete the mark identified by the passed char.
    void deleteMark( QChar mark );
    // Delete the mark present on the passed line or do nothing if there is
    // none.
    void deleteMark( qint64 line );
    // Completely clear the marks list.
    void clear();
};

#endif

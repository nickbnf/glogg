/*
 * Copyright (C) 2015 Nicolas Bonnefon and other contributors
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

#include <QtGlobal>
#include <QVector>

// This class is a list of end of lines position,
// in addition to a list of qint64 (positions within the files)
// it can keep track of whether the final LF was added (for non-LF terminated
// files) and remove it when more data are added.
class LinePositionArray
{
  public:
    // Default constructor
    LinePositionArray() : array()
    { fakeFinalLF_ = false; }
    // Copy constructor
    inline LinePositionArray( const LinePositionArray& orig )
        : array(orig.array)
    { fakeFinalLF_ = orig.fakeFinalLF_; }

    // Add a new line position at the given position
    // Invariant: pos must be greater than the previous one
    // (this is NOT checked!)
    inline void append( qint64 pos )
    {
        if ( fakeFinalLF_ )
            array.pop_back();

        array.append( pos );
    }
    // Size of the array
    inline int size()
    { return array.size(); }
    // Extract an element
    inline qint64 at( int i ) const
    { return array.at( i ); }
    inline qint64 operator[]( int i ) const
    { return array.at( i ); }
    // Set the presence of a fake final LF
    // Must be used after 'append'-ing a fake LF at the end.
    void setFakeFinalLF( bool finalLF=true )
    { fakeFinalLF_ = finalLF; }

    // Add another list to this one, removing any fake LF on this list.
    // Invariant: all pos in other must be greater than any pos in this
    // (this is NOT checked!)
    LinePositionArray& operator+= ( const LinePositionArray& other )
    {
        // If our final LF is fake, we remove it
        if ( fakeFinalLF_ )
            this->array.pop_back();

        // Append the arrays
        this->array += other.array;

        // In case the 'other' object has a fake LF
        this->fakeFinalLF_ = other.fakeFinalLF_;

        return *this;
    }

  private:
    QVector<qint64> array;
    bool fakeFinalLF_;
};



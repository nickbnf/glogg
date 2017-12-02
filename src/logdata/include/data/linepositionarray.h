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

#ifndef LINEPOSITIONARRAY_H
#define LINEPOSITIONARRAY_H

#include <vector>

#include "compressedlinestorage.h"

using  SimpleLinePositionStorage = std::vector<LineOffset>;

// This class is a list of end of lines position,
// in addition to a list of uint64_t (positions within the files)
// it can keep track of whether the final LF was added (for non-LF terminated
// files) and remove it when more data are added.
template <typename Storage>
class LinePosition
{
  public:
    template <typename> friend class LinePosition;

    // Default constructor
    LinePosition() : fakeFinalLF_{ false } {}

    // Copy constructor (slow: deleted)
    LinePosition( const LinePosition& ) = delete;
    LinePosition& operator =(const LinePosition&) = delete;

    // Move assignement
    LinePosition(LinePosition&& orig)
    {
        *this = std::move(orig);
    }

    LinePosition& operator=( LinePosition&& orig )
    {
      array = std::move( orig.array );
      fakeFinalLF_ = orig.fakeFinalLF_;
      return *this;
    }

    // Add a new line position at the given position
    // Invariant: pos must be greater than the previous one
    // (this is NOT checked!)
    inline void append( LineOffset pos )
    {
        LOG(logDEBUG2) << "Next line at " << pos;
        if ( fakeFinalLF_ )
            array.pop_back();

        array.push_back( pos );
        fakeFinalLF_ = false;
    }
    // Size of the array
    inline LinesCount size() const
    { return array.size(); }
    // Extract an element
    inline LineOffset at( LineNumber::UnderlyingType i ) const
    {
        const auto pos = array.at( i );
        LOG(logDEBUG) << "Line pos at " << i << " is " << pos;
        return pos;
    }
    inline LineOffset operator[]( LineNumber::UnderlyingType i ) const
    { return array.at( i ); }

    // Set the presence of a fake final LF
    // Must be used after 'append'-ing a fake LF at the end.
    void setFakeFinalLF( bool finalLF = true )
    { fakeFinalLF_ = finalLF; }

    // Add another list to this one, removing any fake LF on this list.
    // Invariant: all pos in other must be greater than any pos in this
    // (this is NOT checked!)
    void append_list( const LinePosition<SimpleLinePositionStorage>& other )
    {
        // If our final LF is fake, we remove it
        if ( fakeFinalLF_ )
            this->array.pop_back();

        // Append the arrays
        this->array.append_list( other.array );
        //array += other.array;

        // In case the 'other' object has a fake LF
        this->fakeFinalLF_ = other.fakeFinalLF_;
    }

  private:
    Storage array;
    bool fakeFinalLF_;
};

// Use the non-optimised storage
using FastLinePositionArray = LinePosition<SimpleLinePositionStorage>;
//typedef LinePosition<VectorLinePositionStorage> LinePositionArray;

using LinePositionArray = LinePosition<CompressedLinePositionStorage>;

#endif

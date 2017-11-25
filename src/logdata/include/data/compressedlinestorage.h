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

#include <vector>
#include <cstdint>

#include "threadprivatestore.h"
#include "blockpool.h"

// This class is a compressed storage backend for LinePositionArray
// It emulates the interface of a vector, but take advantage of the nature
// of the stored data (increasing end of line addresses) to apply some
// compression in memory, while still providing fast, constant-time look-up.

/* The current algorithm takes advantage of the fact most lines are reasonably
 * short, it codes each line on:
 * - Line < 127 bytes    : 1 byte
 * - 127 < line < 16383  : 2 bytes
 * - line > 16383        : 6 bytes (or 10 bytes)
 * Uncompressed backend stores line on 4 bytes or 8 bytes.
 *
 * The algorithm is quite simple, the file is first divided in two parts:
 * - The lines whose end are located before UINT32_MAX
 * - The lines whose end are located after UINT32_MAX
 * Those end of lines are stored separately in the table32 and the table64
 * respectively.
 *
 * The EOL list is then divided in blocks of BLOCK_SIZE (128) lines.
 * A block index vector (per table) contains pointers to each block.
 *
 * Each block is then defined as such:
 * Block32 (sizes in byte)
 * 00 - Absolute EOF address (4 bytes)
 * 04 - ( 0xxx xxxx  if second line is < 127 ) (1 byte, relative)
 *    - ( 10xx xxxx
 *        xxxx xxxx  if second line is < 16383 ) (2 bytes, relative)
 *    - ( 1111 1111
 *        xxxx xxxx
 *        xxxx xxxx  if second line is > 16383 ) (6 bytes, absolute)
 * ...
 * (126 more lines)
 *
 * Block64 (sizes in byte)
 * 00 - Absolute EOF address (8 bytes)
 * 08 - ( 0xxx xxxx  if second line is < 127 ) (1 byte, relative)
 *    - ( 10xx xxxx
 *        xxxx xxxx  if second line is < 16383 ) (2 bytes, relative)
 *    - ( 1111 1111
 *        xxxx xxxx
 *        xxxx xxxx
 *        xxxx xxxx
 *        xxxx xxxx  if second line is > 16383 ) (10 bytes, absolute)
 * ...
 * (126 more lines)
 *
 * Absolute addressing has been adopted for line > 16383 to bound memory usage in case
 * of pathologically long (MBs or GBs) lines, even if it is a bit less efficient for
 * long-ish (30 KB) lines.
 *
 * The table32 always starts at 0, the table64 starts at first_long_line_
 */

#ifndef COMPRESSEDLINESTORAGE_H
#define COMPRESSEDLINESTORAGE_H

#define BLOCK_SIZE 256

//template<int BLOCK_SIZE = 128>
class CompressedLinePositionStorage
{
  public:
    // Default constructor
    CompressedLinePositionStorage()
    {
        nb_lines_ = 0; first_long_line_ = UINT32_MAX;
        current_pos_ = 0;
        block_index_ = 0;
        long_block_index_ = 0;
        block_offset_ = 0;
        previous_block_offset_ = 0;
    }
    // Copy constructor would be slow, delete!
    CompressedLinePositionStorage( const CompressedLinePositionStorage& orig ) = delete;

    // Move constructor
    CompressedLinePositionStorage( CompressedLinePositionStorage&& orig );
    // Move assignement
    CompressedLinePositionStorage& operator=(
            CompressedLinePositionStorage&& orig );

    // Append the passed end-of-line to the storage
    void append( uint64_t pos );
    void push_back( uint64_t pos )
    { append( pos ); }
    // Size of the array
    uint32_t size() const
    { return nb_lines_; }
    // Element at index
    uint64_t at( uint32_t i ) const;

    // Add one list to the other
    void append_list( const std::vector<uint64_t>& positions );

    // Pop the last element of the storage
    void pop_back();

  private:
    // Utility for move ctor/assign
    void move_from( CompressedLinePositionStorage&& orig );

    // The two indexes
    BlockPool<uint32_t> pool32_;
    BlockPool<uint64_t> pool64_;

    // Total number of lines in storage
    uint32_t nb_lines_;

    // Current position (position of the end of the last line added)
    uint64_t current_pos_;

    uint32_t block_index_;
    uint32_t long_block_index_;
    // Offset of the next position (not yet written) within the current
    // block. null means there is no current block (previous block
    // finished or no data)
    ptrdiff_t block_offset_;


    // The index of the first line whose end is stored in a block64
    // Initialised at UINT32_MAX, meaning "unset"
    // this is the origin point for all calculations in block64
    uint32_t first_long_line_;

    // For pop_back:

    // Previous offset to block element, it is restored when we
    // "pop_back" the last element.
    // A null here means pop_back need to free the block
    // that has just been created.
    ptrdiff_t previous_block_offset_;

    // Cache the last position read
    // This is to speed up consecutive reads (whole page)
    struct Cache {
        Cache()
          : index{UINT32_MAX - 1U}
          , position{0}
          , offset{0}
        {}

        uint32_t index;
        uint64_t position;
        ptrdiff_t offset;
    };
    mutable ThreadPrivateStore<Cache,2> last_read_; // = { UINT32_MAX - 1U, 0, nullptr };
    // mutable Cache last_read;
};

#endif

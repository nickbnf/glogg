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
#include <deque>

#include "threadprivatestore.h"

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

template<typename BlockType>
class BlockPool
{
    using PoolType = std::vector<char>;

public:
    BlockPool()
        : allocationSize_{}
        , lastBlockSize_{}
    {
        blockIndex_.reserve(10000);
        pool_.resize(1024*1024);
    }

    BlockPool(const BlockType&) = delete;
    BlockPool& operator =(const BlockType&) = delete;

    BlockPool(BlockPool&& other)
    {
        *this = std::move(other);
    }

    BlockPool& operator=(BlockPool&& other)
    {
        allocationSize_ = other.allocationSize_;
        pool_ = std::move(other.pool_);
        blockIndex_ = std::move(other.blockIndex_);
        return *this;
    }

    char* get_block(size_t block_size, BlockType initial_position, char** block_ptr)
    {
        auto ptr = get_block( block_size );
        if ( ptr ) {
            *(reinterpret_cast<BlockType*>(ptr)) = initial_position;
            if (block_ptr) {
                *block_ptr = ptr + sizeof(BlockType);
            }
        }

        return ptr;
    }

    char* resize_last_block(size_t new_size)
    {
        const auto aligned_new_size = get_aligned_size(new_size);

        LOG(logWARNING) << "Resize block " << sizeof(BlockType)
                        << " from " << lastBlockSize_
                        << " to " << new_size
                        << " aligned " << aligned_new_size;

        if (aligned_new_size < lastBlockSize_) {
            allocationSize_ -= (lastBlockSize_ - aligned_new_size);
        }
        else {
            const auto delta = aligned_new_size - lastBlockSize_;
            if (allocationSize_ + delta < pool_.size()) {
                pool_.resize(pool_.size() * 2);
            }
            allocationSize_ += delta;
        }

        return pool_.data() + blockIndex_.back();
    }

    void free_last_block()
    {
        LOG(logWARNING) << "Free block " << sizeof(BlockType) << " " << lastBlockSize_;

        if (allocationSize_ >= lastBlockSize_)
        {
            allocationSize_ -= lastBlockSize_;
        }

        blockIndex_.pop_back();
    }

    char* operator[](size_t index)
    {
        return pool_.data() + blockIndex_.at(index);
    }

    const char* operator[](size_t index) const
    {
        return pool_.data() + blockIndex_.at(index);
    }

private:

    size_t get_aligned_size(size_t required_size) const
    {
        return required_size + required_size % alignof(BlockType);
    }

    size_t get_required_size(size_t block_size) const
    {
        return get_aligned_size(sizeof(BlockType) + block_size * (sizeof(BlockType) + 2));
    }

    char* get_block(size_t block_size)
    {
        const auto requiredSize = get_required_size(block_size);

        LOG(logWARNING) << "Get block " << sizeof(BlockType) << " pool " << pool_.size() << " alloc " << allocationSize_;

        if (allocationSize_ + requiredSize >= pool_.size()) {
            pool_.resize(pool_.size() * 2);
        }

        blockIndex_.push_back(allocationSize_);
        allocationSize_ += requiredSize;
        lastBlockSize_ = requiredSize;

        return pool_.data() + blockIndex_.back();
    }

  private:
    std::vector<char> pool_;

    size_t allocationSize_;
    size_t lastBlockSize_;
    std::vector<size_t> blockIndex_;
};

//template<int BLOCK_SIZE = 128>
class CompressedLinePositionStorage
{
  public:
    // Default constructor
    CompressedLinePositionStorage()
    {
        nb_lines_ = 0; first_long_line_ = UINT32_MAX;
        current_pos_ = 0; block_pointer_ = nullptr;
        previous_block_pointer_ = nullptr;
    }
    // Copy constructor would be slow, delete!
    CompressedLinePositionStorage( const CompressedLinePositionStorage& orig ) = delete;

    // Move constructor
    CompressedLinePositionStorage( CompressedLinePositionStorage&& orig );
    // Move assignement
    CompressedLinePositionStorage& operator=(
            CompressedLinePositionStorage&& orig );
    // Destructor
    ~CompressedLinePositionStorage();

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
    void free_blocks();

    // The two indexes
    BlockPool<uint32_t> pool32_;
    BlockPool<uint64_t> pool64_;

    // Total number of lines in storage
    uint32_t nb_lines_;

    // Current position (position of the end of the last line added)
    uint64_t current_pos_;
    // Address of the next position (not yet written) within the current
    // block. nullptr means there is no current block (previous block
    // finished or no data)
    char* block_pointer_;

    // The index of the first line whose end is stored in a block64
    // Initialised at UINT32_MAX, meaning "unset"
    // this is the origin point for all calculations in block64
    uint32_t first_long_line_;

    // For pop_back:

    // Previous pointer to block element, it is restored when we
    // "pop_back" the last element.
    // A null pointer here means pop_back need to free the block
    // that has just been created.
    char* previous_block_pointer_;

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

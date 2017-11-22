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

#include <cassert>
#include <cstdlib>
#include <QtEndian>

#include "utils.h"

#include "data/compressedlinestorage.h"

namespace {
    // Functions to manipulate blocks

    // Add a one byte relative delta (0-127) and inc pointer
    // First bit is always 0
    void block_add_one_byte_relative( uint8_t** ptr, uint8_t value )
    {
        **ptr = value;
        *ptr += sizeof( value );
    }

    // Add a two bytes relative delta (0-16383) and inc pointer
    // First 2 bits are always 10
    void block_add_two_bytes_relative( uint8_t** ptr, uint16_t value )
    {
        // Stored in big endian format in order to recognise the initial pattern:
        // 10xx xxxx xxxx xxxx
        //  HO byte | LO byte
        *(reinterpret_cast<uint16_t*>(*ptr)) = qToBigEndian( static_cast<uint16_t>( value | (1 << 15) ) );
        *ptr += sizeof( value );
    }

    template<typename ElementType>
    void block_add_absolute( uint8_t** ptr, ElementType value )
    {
        // 2 bytes marker (actually only the first two bits are tested)
        *(reinterpret_cast<uint16_t*>(*ptr)) = 0xFF;
        *ptr += sizeof( uint16_t );


        // Absolute value (machine endian)
        // This might be unaligned, can cause problem on some CPUs
        *(reinterpret_cast<ElementType*>(*ptr)) = value;
        *ptr += sizeof( ElementType );
    }

    // Initialise the passed block for reading, returning
    // the initial position and a pointer to the second entry.
    template<typename ElementType>
    uint64_t block_initial_pos( const uint8_t* block, const uint8_t** ptr )
    {
        *ptr = block + sizeof( ElementType );
        return *(reinterpret_cast<const ElementType*>(block));
    }

    // Give the next position in the block based on the previous
    // position, then increase the pointer.
    template<typename ElementType>
    uint64_t block_next_pos( const uint8_t** ptr, uint64_t previous_pos )
    {
        uint64_t pos = previous_pos;

        uint8_t byte = **ptr;
        ++(*ptr);
        if ( ! ( byte & 0x80 ) ) {
            // High order bit is 0
            pos += byte;
        }
        else if ( ( byte & 0xC0 ) == 0x80 ) {
            // We need to read the low order byte
            uint8_t lo_byte = **ptr;
            ++(*ptr);
            // Remove the starting 10b
            byte &= ~0xC0;
            // And form the displacement (stored as big endian)
            pos += ( (uint16_t) byte << 8 ) | (uint16_t) lo_byte;
        }
        else {
            // Skip the next byte (not used)
            ++(*ptr);
            // And read the new absolute pos (machine endian)
            pos = *(reinterpret_cast<const ElementType*>(*ptr));
            *ptr += sizeof( ElementType );
        }

        return pos;
    }
}

void CompressedLinePositionStorage::move_from(
        CompressedLinePositionStorage&& orig )
{
    nb_lines_        = orig.nb_lines_;
    first_long_line_ = orig.first_long_line_;
    current_pos_     = orig.current_pos_;
    block_pointer_   = orig.block_pointer_;
    previous_block_pointer_ = orig.previous_block_pointer_;

    orig.nb_lines_   = 0;
}

// Move constructor
CompressedLinePositionStorage::CompressedLinePositionStorage(
        CompressedLinePositionStorage&& orig )
    : pool32_( std::move( orig.pool32_ ) ),
      pool64_( std::move( orig.pool64_ ) )
{
    move_from( std::move( orig ) );
}

// Move assignement
CompressedLinePositionStorage& CompressedLinePositionStorage::operator=(
        CompressedLinePositionStorage&& orig )
{
    pool32_ = std::move( orig.pool32_ );
    pool64_ = std::move( orig.pool64_ );
    move_from( std::move( orig ) );

    return *this;
}

// template<int BLOCK_SIZE>
void CompressedLinePositionStorage::append( uint64_t pos )
{
    // Lines must be stored in order
    assert( ( pos > current_pos_ ) || ( pos == 0 ) );

    // Save the pointer in case we need to "pop_back"
    previous_block_pointer_ = block_pointer_;

    bool store_in_big = false;
    if ( pos > UINT32_MAX ) {
        store_in_big = true;
        if ( first_long_line_ == UINT32_MAX ) {
            // First "big" end of line, we will start a new (64) block
            first_long_line_ = nb_lines_;
            block_pointer_ = nullptr;
        }
    }

    if ( ! block_pointer_ ) {
        // We need to start a new block
        if ( ! store_in_big )
            pool32_.get_block( BLOCK_SIZE, pos, &block_pointer_ );
        else
           pool64_.get_block( BLOCK_SIZE, pos, &block_pointer_ );
    }
    else {
        uint64_t delta = pos - current_pos_;
        if ( delta < 128 ) {
            // Code relative on one byte
            block_add_one_byte_relative( &block_pointer_, delta );
        }
        else if ( delta < 16384 ) {
            // Code relative on two bytes
            block_add_two_bytes_relative( &block_pointer_, delta );
        }
        else {
            // Code absolute
            if ( ! store_in_big )
                block_add_absolute<uint32_t>( &block_pointer_, static_cast<uint32_t>( pos ) );
            else
                block_add_absolute<uint64_t>( &block_pointer_, pos );
        }
    }

    current_pos_ = pos;
    ++nb_lines_;

    const auto shrinkBlock = [this]( auto& blockPool, int block_index )
    {
        const auto block = blockPool[block_index];
        const auto effective_block_size = std::distance( block, previous_block_pointer_ );

        // We allocate extra space for the last element in case it
        // is replaced by an absolute value in the future (following a pop_back)
        const auto new_size = effective_block_size + blockPool.getPaddedElementSize();
        const auto new_location = blockPool.resize_last_block(new_size);

        block_pointer_ = nullptr;
        previous_block_pointer_ = new_location + effective_block_size;
    };

    if ( ! store_in_big ) {
        if ( nb_lines_ % BLOCK_SIZE == 0 ) {
            // We have finished the block

            // Let's reduce its size to what is actually used
            const auto block_index = nb_lines_ / BLOCK_SIZE - 1;
            shrinkBlock( pool32_, block_index );
        }
    }
    else {
        if ( ( nb_lines_ - first_long_line_ ) % BLOCK_SIZE == 0 ) {
            // We have finished the block

            // Let's reduce its size to what is actually used
            const auto block_index = ( nb_lines_ - first_long_line_ ) / BLOCK_SIZE - 1;
            shrinkBlock( pool64_, block_index );
        }
    }
}

// template<int BLOCK_SIZE>
uint64_t CompressedLinePositionStorage::at( uint32_t index ) const
{
    const uint8_t* block;
    const uint8_t* ptr;
    uint64_t position;

    Cache* last_read = last_read_.getPtr();

    if ( index < first_long_line_ ) {
        block = pool32_[ index / BLOCK_SIZE ];

        if ( ( index == last_read->index + 1 ) && ( index % BLOCK_SIZE != 0 ) ) {
            position = last_read->position;
            ptr      =  block + last_read->offset;

            position = block_next_pos<uint32_t>( &ptr, position );
        }
        else {
            position = block_initial_pos<uint32_t>( block, &ptr );

            for ( uint32_t i = 0; i < index % BLOCK_SIZE; i++ ) {
                // Go through all the lines in the block till the one we want
                position = block_next_pos<uint32_t>( &ptr, position );
            }
        }
    }
    else {
        const uint32_t index_in_64 = index - first_long_line_;
        block = pool64_[ index_in_64 / BLOCK_SIZE ];

        if ( ( index == last_read->index + 1 ) && ( index_in_64 % BLOCK_SIZE != 0 ) ) {
            position = last_read->position;
            ptr      = block + last_read->offset;

            position = block_next_pos<uint64_t>( &ptr, position );
        }
        else {
            position = block_initial_pos<uint64_t>( block, &ptr );

            for ( uint32_t i = 0; i < index_in_64 % BLOCK_SIZE; i++ ) {
                // Go through all the lines in the block till the one we want
                position = block_next_pos<uint64_t>( &ptr, position );
            }
        }
    }

    // Populate our cache ready for next consecutive read
    last_read->index    = index;
    last_read->position = position;
    last_read->offset   = std::distance( block, ptr );

    return position;
}

void CompressedLinePositionStorage::append_list(
        const std::vector<uint64_t>& positions )
{
    // This is not very clever, but caching should make it
    // reasonably fast.
    for ( uint32_t i = 0; i < positions.size(); i++ )
        append( positions.at( i ) );
}

// template<int BLOCK_SIZE>
void CompressedLinePositionStorage::pop_back()
{
    // Removing the last entered data, there are two cases
    if ( previous_block_pointer_ ) {
        // The last append was a normal entry in an existing block,
        // so we can just revert the pointer
        block_pointer_ = previous_block_pointer_;
        previous_block_pointer_ = nullptr;
    }
    else {
        // A new block has been created for the last entry, we need
        // to de-alloc it.

        if ( first_long_line_ == UINT32_MAX ) {
            // If we try to pop_back() twice, we're dead!
            assert( ( nb_lines_ - 1 ) % BLOCK_SIZE == 0 );

            pool32_.free_last_block();
        }
        else {
            // If we try to pop_back() twice, we're dead!
            assert( ( nb_lines_ - first_long_line_ - 1 ) % BLOCK_SIZE == 0 );

            pool64_.free_last_block();
        }

        block_pointer_ = nullptr;
    }

    --nb_lines_;
    current_pos_ = at( nb_lines_ - 1 );
}

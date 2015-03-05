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

#include <cstdlib>
#include <arpa/inet.h>

#include <iostream>

#include "data/compressedlinestorage.h"

namespace {
    // Functions to manipulate blocks

    // Create a new 32 bits block of the passed size,
    // initialised at the passed position
    char* block32_new( int block_size, uint32_t initial_position,
           char** block_ptr )
    {
        // malloc a block of the maximum possible size
        // (where every line is >16384)
        char* ptr = static_cast<char*>( malloc( 4 + block_size * 6 ) );

        if ( ptr ) {
            // Write the initial_position
            *(reinterpret_cast<uint32_t*>(ptr)) = initial_position;
            *block_ptr = ptr + 4;
        }

        return ptr;
    }

    // Add a one byte relative delta (0-127) and inc pointer
    // First bit is always 0
    void block_add_one_byte_relative( char** ptr, uint8_t value )
    {
        **ptr = value;
        *ptr += sizeof( value );
    }

    // Add a two bytes relative delta (0-16383) and inc pointer
    // First 2 bits are always 10
    void block_add_two_bytes_relative( char** ptr, uint16_t value )
    {
        // Stored in big endian format in order to recognise the initial pattern:
        // 10xx xxxx xxxx xxxx
        //  HO byte | LO byte
        *(reinterpret_cast<uint16_t*>(*ptr)) = htons( value | (1 << 15) );
        *ptr += sizeof( value );
    }

    // Add a signal and a 4 bytes absolute position and inc pointer
    void block32_add_absolute( char** ptr, uint32_t value )
    {
        // 2 bytes marker (actually only the first two bits are tested)
        *(reinterpret_cast<uint16_t*>(*ptr)) = 0xFF;
        *ptr += sizeof( uint16_t );
        // Absolute value (machine endian)
        *(reinterpret_cast<uint32_t*>(*ptr)) = value;
        *ptr += sizeof( uint32_t );
    }

    // Initialise the passed block for reading, returning
    // the initial position and a pointer to the second entry.
    uint64_t block32_initial_pos( char* block, char** ptr )
    {
        *ptr = block + sizeof( uint32_t );
        return *(reinterpret_cast<uint32_t*>(block));
    }

    // Give the next position in the block based on the previous
    // position, then increase the pointer.
    uint64_t block32_next_pos( char** ptr, uint64_t previous_pos )
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
            pos = *(reinterpret_cast<uint32_t*>(*ptr));
            *ptr += sizeof( uint32_t );
        }

        return pos;
    }
}

// template<int BLOCK_SIZE>
void CompressedLinePositionStorage::append( uint64_t pos )
{
    if ( ! block_pointer_ ) {
        // We need to start a new block
        block32_index_.push_back(
            block32_new( BLOCK_SIZE, pos, &block_pointer_ ) );
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
            block32_add_absolute( &block_pointer_, pos );
        }
    }

    current_pos_ = pos;
    ++nb_lines_;

    if ( nb_lines_ % BLOCK_SIZE == 0 ) {
        // We have finished the block
        block_pointer_ = nullptr;
    }
}

// template<int BLOCK_SIZE>
uint64_t CompressedLinePositionStorage::at( int index ) const
{
    char* block = block32_index_[ index / BLOCK_SIZE ];
    char* ptr;
    uint64_t position = block32_initial_pos( block, &ptr );

    for ( int i = 0; i < index % BLOCK_SIZE; i++ ) {
        // Go through all the lines in the block till the one we want
        position = block32_next_pos( &ptr, position );
    }

    return position;
}

void CompressedLinePositionStorage::append_list(
        const CompressedLinePositionStorage& other )
{
    // This is not very clever, but caching should make it
    // reasonably fast.
    for ( uint32_t i = 0; i < other.size(); i++ )
        append( other.at( i ) );
}

// template<int BLOCK_SIZE>
void CompressedLinePositionStorage::pop_back()
{
}

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
#include <iostream>

#include "utils.h"

#include "data/compressedlinestorage.h"

#ifdef HAVE_HTONS
#  include <arpa/inet.h>
#else
#  define htons(a) glogg_htons(a)
#endif

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

    // Create a new 64 bits block of the passed size,
    // initialised at the passed position
    char* block64_new( int block_size, uint64_t initial_position,
           char** block_ptr )
    {
        // malloc a block of the maximum possible size
        // (where every line is >16384)
        char* ptr = static_cast<char*>( malloc( 8 + block_size * 10 ) );

        if ( ptr ) {
            // Write the initial_position
            *(reinterpret_cast<uint64_t*>(ptr)) = initial_position;
            *block_ptr = ptr + 8;
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

    // Add a signal and a 8 bytes absolute position and inc pointer
    void block64_add_absolute( char** ptr, uint64_t value )
    {
        // This is unaligned, can cause problem on some CPUs

        // 2 bytes marker (actually only the first two bits are tested)
        *(reinterpret_cast<uint16_t*>(*ptr)) = 0xFF;
        *ptr += sizeof( uint16_t );
        // Absolute value (machine endian)
        *(reinterpret_cast<uint64_t*>(*ptr)) = value;
        *ptr += sizeof( uint64_t );
    }

    // Initialise the passed block for reading, returning
    // the initial position and a pointer to the second entry.
    uint64_t block64_initial_pos( char* block, char** ptr )
    {
        *ptr = block + sizeof( uint64_t );
        return *(reinterpret_cast<uint64_t*>(block));
    }

    // Give the next position in the block based on the previous
    // position, then increase the pointer.
    uint64_t block64_next_pos( char** ptr, uint64_t previous_pos )
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
            pos = *(reinterpret_cast<uint64_t*>(*ptr));
            *ptr += sizeof( uint64_t );
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
    : block32_index_( std::move( orig.block32_index_ ) )
{
    move_from( std::move( orig ) );
}

// Move assignement
CompressedLinePositionStorage& CompressedLinePositionStorage::operator=(
        CompressedLinePositionStorage&& orig )
{
    block32_index_ = std::move( orig.block32_index_ );
    move_from( std::move( orig ) );

    return *this;
}

CompressedLinePositionStorage::~CompressedLinePositionStorage()
{
    for ( char* block : block32_index_ ) {
        void* p = static_cast<void*>( block );
        // std::cerr << "block = " << p << std::endl;
        free( p );
    }

    for ( char* block : block64_index_ ) {
        void* p = static_cast<void*>( block );
        // std::cerr << "block = " << p << std::endl;
        free( p );
    }
}

// template<int BLOCK_SIZE>
void CompressedLinePositionStorage::append( uint64_t pos )
{
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
            block32_index_.push_back(
                block32_new( BLOCK_SIZE, pos, &block_pointer_ ) );
        else
            block64_index_.push_back(
                block64_new( BLOCK_SIZE, pos, &block_pointer_ ) );
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
                block32_add_absolute( &block_pointer_, pos );
            else
                block64_add_absolute( &block_pointer_, pos );
        }
    }

    current_pos_ = pos;
    ++nb_lines_;

    if ( ! store_in_big ) {
        if ( nb_lines_ % BLOCK_SIZE == 0 ) {
            // We have finished the block

            // Let's reduce its size to what is actually used
            int block_index = nb_lines_ / BLOCK_SIZE - 1;
            char* block = block32_index_[block_index];
            // We allocate extra space for the last element in case it
            // is replaced by an absolute value in the future (following a pop_back)
            size_t new_size = ( previous_block_pointer_
                    + sizeof( uint16_t ) + sizeof( uint32_t ) ) - block;
            void* new_location = realloc( block, new_size );
            if ( new_location )
                block32_index_[block_index] = static_cast<char*>( new_location );

            block_pointer_ = nullptr;
        }
    }
    else {
        if ( ( nb_lines_ - first_long_line_ ) % BLOCK_SIZE == 0 ) {
            // We have finished the block

            // Let's reduce its size to what is actually used
            int block_index = ( nb_lines_ - first_long_line_ ) / BLOCK_SIZE - 1;
            char* block = block64_index_[block_index];
            // We allocate extra space for the last element in case it
            // is replaced by an absolute value in the future (following a pop_back)
            size_t new_size = ( previous_block_pointer_
                    + sizeof( uint16_t ) + sizeof( uint64_t ) ) - block;
            void* new_location = realloc( block, new_size );
            if ( new_location )
                block64_index_[block_index] = static_cast<char*>( new_location );

            block_pointer_ = nullptr;
        }
    }
}

// template<int BLOCK_SIZE>
uint64_t CompressedLinePositionStorage::at( uint32_t index ) const
{
    char* ptr;
    uint64_t position;

    Cache* last_read = last_read_.getPtr();

    if ( index < first_long_line_ ) {
        if ( ( index == last_read->index + 1 ) && ( index % BLOCK_SIZE != 0 ) ) {
            position = last_read->position;
            ptr      = last_read->ptr;

            position = block32_next_pos( &ptr, position );
        }
        else {
            char* block = block32_index_[ index / BLOCK_SIZE ];
            position = block32_initial_pos( block, &ptr );

            for ( uint32_t i = 0; i < index % BLOCK_SIZE; i++ ) {
                // Go through all the lines in the block till the one we want
                position = block32_next_pos( &ptr, position );
            }
        }
    }
    else {
        const uint32_t index_in_64 = index - first_long_line_;
        if ( ( index == last_read->index + 1 ) && ( index_in_64 % BLOCK_SIZE != 0 ) ) {
            position = last_read->position;
            ptr      = last_read->ptr;

            position = block64_next_pos( &ptr, position );
        }
        else {
            char* block = block64_index_[ index_in_64 / BLOCK_SIZE ];
            position = block64_initial_pos( block, &ptr );

            for ( uint32_t i = 0; i < index_in_64 % BLOCK_SIZE; i++ ) {
                // Go through all the lines in the block till the one we want
                position = block64_next_pos( &ptr, position );
            }
        }
    }

    // Populate our cache ready for next consecutive read
    last_read->index    = index;
    last_read->position = position;
    last_read->ptr      = ptr;

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

        // If we try to pop_back() twice, we're dead!
        assert( ( nb_lines_ - 1 ) % BLOCK_SIZE == 0 );

        char* block = block32_index_.back();
        block32_index_.pop_back();
        free( block );


        block_pointer_ = nullptr;
    }

    --nb_lines_;
    current_pos_ = at( nb_lines_ - 1 );
}

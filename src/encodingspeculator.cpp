/*
 * Copyright (C) 2016 Nicolas Bonnefon and other contributors
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

#include "encodingspeculator.h"

#include <iostream>

void EncodingSpeculator::inject_byte( uint8_t byte )
{
    if ( ! ( byte & 0x80 ) ) {
        // 7-bit character, all fine
    }
    else {
        switch ( state_ ) {
            case State::ASCIIOnly:
            case State::ValidUTF8:
                if ( ( byte & 0xE0 ) == 0xC0 ) {
                    state_ = State::UTF8LeadingByteSeen;
                    code_point_ = ( byte & 0x1F ) << 6;
                    continuation_left_ = 1;
                    min_value_ = 0x80;
                    // std::cout << "Lead: cp= " << std::hex << code_point_ << std::endl;
                }
                else if ( ( byte & 0xF0 ) == 0xE0 ) {
                    state_ = State::UTF8LeadingByteSeen;
                    code_point_ = ( byte & 0x0F ) << 12;
                    continuation_left_ = 2;
                    min_value_ = 0x800;
                    // std::cout << "Lead 3: cp= " << std::hex << code_point_ << std::endl;
                }
                else if ( ( byte & 0xF8 ) == 0xF0 ) {
                    state_ = State::UTF8LeadingByteSeen;
                    code_point_ = ( byte & 0x07 ) << 18;
                    continuation_left_ = 3;
                    min_value_ = 0x800;
                    // std::cout << "Lead 4: cp= " << std::hex << code_point_ << std::endl;
                }
                else {
                    state_ = State::Unknown8Bit;
                }
                break;
            case State::UTF8LeadingByteSeen:
                if ( ( byte & 0xC0 ) == 0x80 ) {
                    --continuation_left_;
                    code_point_ |= ( byte & 0x3F ) << (continuation_left_ * 6);
                    // std::cout << "Cont: cp= " << std::hex << code_point_ << std::endl;
                    if ( continuation_left_ == 0 ) {
                        if ( code_point_ >= min_value_ )
                            state_ = State::ValidUTF8;
                        else
                            state_ = State::Unknown8Bit;
                    }
                }
                else {
                    state_ = State::Unknown8Bit;
                }
                break;
        }
        // state_ = State::Unknown8Bit;
    }
}

EncodingSpeculator::Encoding EncodingSpeculator::guess() const
{
    Encoding guess;

    switch ( state_ ) {
        case State::ASCIIOnly:
            guess = Encoding::ASCII7;
            break;
        case State::Unknown8Bit:
        case State::UTF8LeadingByteSeen:
            guess = Encoding::ASCII8;
            break;
        case State::ValidUTF8:
            guess = Encoding::UTF8;
            break;
        default:
            guess = Encoding::ASCII8;
    }

    return guess;
}

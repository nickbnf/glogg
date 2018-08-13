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

#ifndef ENCODINGSPECULATOR_H
#define ENCODINGSPECULATOR_H

#include <cstdint>

// The encoder speculator tries to determine the likely encoding
// of the stream of bytes which is passed to it.

class EncodingSpeculator {
  public:
    enum class Encoding {
        ASCII7,
        ASCII8,
        UTF8,
        UTF16LE,
        UTF16BE,
        BIG5,
        GB18030,
        SHIFT_JIS,
        KOI8R
    };

    EncodingSpeculator() : state_( State::Start ) {}

    // Inject one byte into the speculator
    void inject_byte( uint8_t byte );

    // Returns the current guess based on the previously injected bytes
    Encoding guess() const;

  private:
    enum class State {
        Start,
        ASCIIOnly,
        OtherOrUnknown8Bit,
        UTF8LeadingByteSeen,
        ValidUTF8,
        UTF16BELeadingBOMByteSeen,
        UTF16LELeadingBOMByteSeen,
        ValidUTF16LE,
        ValidUTF16BE,
    };

    State state_;
    uint32_t code_point_;
    int continuation_left_;
    uint32_t min_value_;
};

#endif

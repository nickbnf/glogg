#include "gmock/gmock.h"

#include "config.h"

#include "log.h"

#include "encodingspeculator.h"

using namespace std;
using namespace testing;

class EncodingSpeculatorBehaviour: public testing::Test {
  public:
    EncodingSpeculator speculator;

    EncodingSpeculatorBehaviour() {
    }
};

TEST_F( EncodingSpeculatorBehaviour, DefaultAsPureAscii ) {
    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::ASCII7 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecognisePureAscii ) {
    for ( uint8_t i = 0; i < 127; ++i )
        speculator.inject_byte( i );

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::ASCII7 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecogniseRandom8bitEncoding ) {
    for ( uint8_t i = 0; i < 127; ++i )
        speculator.inject_byte( i );
    speculator.inject_byte( 0xFF );

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::ASCII8 ) );
}

pair<uint8_t,uint8_t> utf8encode2bytes( uint16_t code_point )
{
    uint8_t cp_low = static_cast<uint8_t>(code_point & 0xFF);
    uint8_t cp_hi  = static_cast<uint8_t>((code_point & 0xFF00) >> 8);
    uint8_t first_byte = 0xC0 | ( ( cp_hi & 0x7F ) << 2 ) | ( ( cp_low & 0xC0 ) >> 6 );
    uint8_t second_byte = 0x80 | ( cp_low & 0x3F );

    return { first_byte, second_byte };
}

vector<uint8_t> utf8encodeMultiBytes( uint32_t code_point )
{
    vector<uint8_t> bytes = {};

    if ( code_point <= 0xFFFF ) {
        uint8_t lead = static_cast<uint8_t>( 0xE0 | ( ( code_point & 0xF000 ) >> 12 ) );
        bytes.push_back( lead );
        bytes.push_back( 0x80 | ( code_point & 0x0FC0 ) >> 6 );
        bytes.push_back( 0x80 | ( code_point & 0x3F ) );
    }
    else if ( code_point <= 0x1FFFFF ) {
        uint8_t lead = static_cast<uint8_t>( 0xF0 | ( ( code_point & 0x1C0000 ) >> 18 ) );
        bytes.push_back( lead );
        bytes.push_back( 0x80 | ( code_point & 0x3F000 ) >> 12 );
        bytes.push_back( 0x80 | ( code_point & 0x00FC0 ) >> 6 );
        bytes.push_back( 0x80 | ( code_point & 0x0003F ) );
    }

    return bytes;
}


TEST_F( EncodingSpeculatorBehaviour, RecogniseTwoBytesUTF8 ) {
    // All the code points encodable as 2 bytes.
    for ( uint16_t i = 0x80; i < ( 1 << 11 ); ++i ) {
        auto utf8_bytes = utf8encode2bytes( i );

        // cout << bitset<8>(first_byte) << " " << bitset<8>(second_byte) << endl;

        speculator.inject_byte( utf8_bytes.first );
        speculator.inject_byte( utf8_bytes.second );
    }

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::UTF8 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecogniseTwoBytesUTF8With7bitsInterleaved ) {
    // All the code points encodable as 2 bytes.
    for ( uint16_t i = 0x80; i < ( 1 << 11 ); ++i ) {
        auto utf8_bytes = utf8encode2bytes( i );

        speculator.inject_byte( ' ' );
        speculator.inject_byte( utf8_bytes.first );
        speculator.inject_byte( utf8_bytes.second );
    }

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::UTF8 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecogniseIncompleteTwoBytesUTF8 ) {
    // All the code points encodable as 2 bytes.
    for ( uint16_t i = 0x80; i < ( 1 << 11 ); ++i ) {
        auto utf8_bytes = utf8encode2bytes( i );

        speculator.inject_byte( ' ' );
        speculator.inject_byte( utf8_bytes.first );
        speculator.inject_byte( utf8_bytes.second );
    }

    // Lead byte only
    speculator.inject_byte( 0xCF );

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::ASCII8 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecogniseIncorrectTwoBytesUTF8 ) {
    // All the code points encodable as 2 bytes.
    for ( uint16_t i = 0x80; i < ( 1 << 11 ); ++i ) {
        auto utf8_bytes = utf8encode2bytes( i );

        speculator.inject_byte( ' ' );
        speculator.inject_byte( utf8_bytes.first );
        speculator.inject_byte( utf8_bytes.second );
    }

    // Lead byte
    speculator.inject_byte( 0xCF );
    // Incorrect continuation byte (should start with 1)
    speculator.inject_byte( 0x00 );

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::ASCII8 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecogniseOverlong2BytesUTF8 ) {
    speculator.inject_byte( ' ' );
    speculator.inject_byte( 0xC1 );
    speculator.inject_byte( 0xBF );

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::ASCII8 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecogniseThreeBytesUTF8 ) {
    for ( uint32_t i = 0x800; i <= 0xFFFF; ++i ) {
        auto utf8_bytes = utf8encodeMultiBytes( i );

        speculator.inject_byte( ' ' );
        for ( uint8_t byte: utf8_bytes ) {
            // cout << hex << i << " " << static_cast<uint32_t>( byte ) << endl;
            speculator.inject_byte( byte );
        }
    }

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::UTF8 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecogniseOverlong3BytesUTF8 ) {
    speculator.inject_byte( ' ' );
    speculator.inject_byte( 0xA0 );
    speculator.inject_byte( 0x80 );
    speculator.inject_byte( 0x80 );

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::ASCII8 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecogniseFourBytesUTF8 ) {
    for ( uint32_t i = 0x10000; i <= 0x1FFFFF; ++i ) {
        auto utf8_bytes = utf8encodeMultiBytes( i );

        speculator.inject_byte( ' ' );
        for ( uint8_t byte: utf8_bytes ) {
            // cout << hex << i << " " << static_cast<uint32_t>( byte ) << endl;
            speculator.inject_byte( byte );
        }
    }

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::UTF8 ) );
}

TEST_F( EncodingSpeculatorBehaviour, RecogniseOverlong4BytesUTF8 ) {
    speculator.inject_byte( ' ' );
    speculator.inject_byte( 0xF0 );
    speculator.inject_byte( 0x80 );
    speculator.inject_byte( 0x80 );
    speculator.inject_byte( 0x80 );

    ASSERT_THAT( speculator.guess(), Eq( EncodingSpeculator::Encoding::ASCII8 ) );
}


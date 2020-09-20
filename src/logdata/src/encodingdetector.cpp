/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "encodingdetector.h"

#include <QTextCodec>

#include "log.h"
#include "uchardet/uchardet.h"

namespace {

class UchardetHolder {
  public:
    UchardetHolder()
        : ud_{ uchardet_new() }
    {
    }
    ~UchardetHolder()
    {
        uchardet_delete( ud_ );
    }

    UchardetHolder( const UchardetHolder& ) = delete;
    UchardetHolder& operator=( const UchardetHolder& ) = delete;

    UchardetHolder( UchardetHolder&& other ) = delete;
    UchardetHolder& operator=( UchardetHolder&& other ) = delete;

    int handle_data( const char* data, size_t len )
    {
        return uchardet_handle_data( ud_, data, len );
    }

    void data_end()
    {
        uchardet_data_end( ud_ );
    }

    const char* get_charset()
    {
        return uchardet_get_charset( ud_ );
    }

  private:
    uchardet_t ud_;
};

} // namespace

EncodingParameters::EncodingParameters( const QTextCodec* codec )
{
    static constexpr QChar lineFeed( QChar::LineFeed );
    QTextCodec::ConverterState convertState( QTextCodec::IgnoreHeader );
    QByteArray encodedLineFeed = codec->fromUnicode( &lineFeed, 1, &convertState );

    lineFeedWidth = encodedLineFeed.length();
    lineFeedIndex = encodedLineFeed[ 0 ] == '\n' ? 0 : ( encodedLineFeed.length() - 1 );
}

QTextCodec* EncodingDetector::detectEncoding( const QByteArray& block ) const
{
    ScopedLock lock( &mutex_ );

    UchardetHolder ud;

    auto rc = ud.handle_data( block.data(), static_cast<size_t>( block.size() ) );
    if ( rc == 0 ) {
        ud.data_end();
    }

    QTextCodec* uchardetCodec = nullptr;
    if ( rc == 0 ) {
        auto uchardetGuess = ud.get_charset();
        LOG( logDEBUG ) << "Uchardet encoding guess " << uchardetGuess;
        uchardetCodec = QTextCodec::codecForName( uchardetGuess );
        if ( uchardetCodec ) {
            LOG( logDEBUG ) << "Uchardet codec selected " << uchardetCodec->name().constData();
        }
        else {
            LOG( logDEBUG ) << "Uchardet codec not found for guess " << uchardetGuess;
        }
    }

    auto encodingGuess = uchardetCodec ? QTextCodec::codecForUtfText( block, uchardetCodec )
                                       : QTextCodec::codecForUtfText( block );

    LOG( logDEBUG ) << "Final encoding guess " << encodingGuess->name().constData();

    return encodingGuess;
}

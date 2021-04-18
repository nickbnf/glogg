/*
 * Copyright (C) 2021 Anton Filimonov and other contributors
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

#ifdef KLOGG_HAS_HS
#include "hsregularexpression.h"

#include "log.h"

namespace {
int matchCallback( unsigned int id, unsigned long long from, unsigned long long to,
                   unsigned int flags, void* context )
{
    Q_UNUSED( id );
    Q_UNUSED( from );
    Q_UNUSED( to );
    Q_UNUSED( flags );
    Q_UNUSED( context );

    return 1;
}
} // namespace

HsMatcher::HsMatcher( const hs_database_t* db, hs_scratch_t* scratch )
    : database_{ db }
    , scratch_{ scratch }
{
}

HsMatcher::~HsMatcher()
{
    if ( scratch_ ) {
        hs_free_scratch( scratch_ );
    }
}

HsMatcher::HsMatcher( HsMatcher&& other )
{
    *this = std::move( other );
}

HsMatcher& HsMatcher::operator=( HsMatcher&& other )
{
    if ( this != &other ) {
        database_ = other.database_;
        scratch_ = std::exchange( other.scratch_, nullptr );
    }

    return *this;
}

bool HsMatcher::hasMatch( const QString& data ) const
{
    if ( !scratch_ ) {
        return false;
    }

    const auto utf8Data = data.toUtf8();
    const auto result
        = hs_scan( database_, utf8Data.data(), static_cast<unsigned int>( utf8Data.size() ), 0,
                   scratch_, matchCallback, nullptr );

    return result == HS_SCAN_TERMINATED;
}

HsRegularExpression::HsRegularExpression( const QRegularExpression& pattern )
{
    hs_database_t* db = nullptr;
    hs_compile_error_t* error = nullptr;

    unsigned flags = HS_FLAG_UTF8 | HS_FLAG_UCP | HS_FLAG_ALLOWEMPTY | HS_FLAG_SINGLEMATCH;
    if ( pattern.patternOptions().testFlag( QRegularExpression::CaseInsensitiveOption ) ) {
        flags |= HS_FLAG_CASELESS;
    }

    const auto compileResult = hs_compile( pattern.pattern().toUtf8().data(), flags, HS_MODE_BLOCK,
                                           nullptr, &db, &error );

    if ( compileResult != HS_SUCCESS ) {
        LOG_ERROR << "Failed to compile pattern " << error->message;
        errorMessage_ = error->message;
        hs_free_compile_error( error );
    }

    hs_scratch_t* scratch = nullptr;
    if ( db ) {
        const auto scratchResult = hs_alloc_scratch( db, &scratch );
        if ( scratchResult != HS_SUCCESS ) {
            LOG_ERROR << "Failed to allocate scratch";
            hs_free_database( db );
        }
    }

    if ( db && scratch ) {
        database_ = db;
        scratch_ = scratch;
    }
}

HsRegularExpression::~HsRegularExpression()
{
    if ( database_ ) {
        hs_free_database( database_ );
    }
    if ( scratch_ ) {
        hs_free_scratch( scratch_ );
    }
}

HsRegularExpression::HsRegularExpression( HsRegularExpression&& other )
{
    *this = std::move( other );
}

HsRegularExpression& HsRegularExpression::operator=( HsRegularExpression&& other )
{
    if ( this != &other ) {
        database_ = std::exchange( other.database_, nullptr );
        scratch_ = std::exchange( other.scratch_, nullptr );
    }

    return *this;
}

bool HsRegularExpression::isValid() const
{
    return database_ != nullptr && scratch_ != nullptr;
}

QString HsRegularExpression::errorString() const
{
    return errorMessage_;
}

HsMatcher HsRegularExpression::createMatcher() const
{
    if ( !isValid() ) {
        return {};
    }

    hs_scratch_t* matcherScratch = nullptr;

    const auto err = hs_clone_scratch( scratch_, &matcherScratch );
    if ( err != HS_SUCCESS ) {
        LOG_ERROR << "hs_clone_scratch failed";
        matcherScratch = nullptr;
    }

    return HsMatcher{ database_, matcherScratch };
}
#endif

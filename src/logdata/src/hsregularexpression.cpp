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

HsMatcher::HsMatcher( HsDatabase db, HsScratch scratch )
    : database_{ std::move( db ) }
    , scratch_{ std::move( scratch ) }
{
}

bool HsMatcher::hasMatch( const QString& data ) const
{
    if ( !scratch_ || !database_ ) {
        return false;
    }

    const auto utf8Data = data.toUtf8();
    const auto result
        = hs_scan( database_.get(), utf8Data.data(), static_cast<unsigned int>( utf8Data.size() ),
                   0, scratch_.get(), matchCallback, nullptr );

    return result == HS_SCAN_TERMINATED;
}

HsRegularExpression::HsRegularExpression( const QRegularExpression& pattern )
{
    database_ = HsDatabase{ wrapHsPointer<hs_database_t, hs_free_database>(
        []( const QRegularExpression& regexp, QString& errorMessage ) -> hs_database_t* {
            hs_database_t* db = nullptr;
            hs_compile_error_t* error = nullptr;

            unsigned flags = HS_FLAG_UTF8 | HS_FLAG_UCP | HS_FLAG_ALLOWEMPTY | HS_FLAG_SINGLEMATCH;
            if ( regexp.patternOptions().testFlag( QRegularExpression::CaseInsensitiveOption ) ) {
                flags |= HS_FLAG_CASELESS;
            }

            const auto compileResult = hs_compile( regexp.pattern().toUtf8().data(), flags,
                                                   HS_MODE_BLOCK, nullptr, &db, &error );

            if ( compileResult != HS_SUCCESS ) {
                LOG_ERROR << "Failed to compile pattern " << error->message;
                errorMessage = error->message;
                hs_free_compile_error( error );
                return nullptr;
            }

            return db;
        },
        pattern, errorMessage_ ) };

    if ( database_ ) {
        scratch_ = wrapHsPointer<hs_scratch_t, hs_free_scratch>(
            []( hs_database_t* db ) -> hs_scratch_t* {
                hs_scratch_t* scratch = nullptr;

                const auto scratchResult = hs_alloc_scratch( db, &scratch );
                if ( scratchResult != HS_SUCCESS ) {
                    LOG_ERROR << "Failed to allocate scratch";
                    return nullptr;
                }

                return scratch;
            },
            database_.get() );
    }
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

    auto matcherScratch = wrapHsPointer<hs_scratch_t, hs_free_scratch>(
        []( hs_scratch_t* prototype ) -> hs_scratch_t* {
            hs_scratch_t* scratch = nullptr;

            const auto err = hs_clone_scratch( prototype, &scratch );
            if ( err != HS_SUCCESS ) {
                LOG_ERROR << "hs_clone_scratch failed";
                return nullptr;
            }

            return scratch;
        },
        scratch_.get() );

    return HsMatcher{ database_, std::move( matcherScratch ) };
}
#endif

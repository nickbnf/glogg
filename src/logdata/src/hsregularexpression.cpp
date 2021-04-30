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

static constexpr auto ExcludeMatchIdStart = 1000;

int matchCallback( unsigned int id, unsigned long long from, unsigned long long to,
                   unsigned int flags, void* context )
{
    Q_UNUSED( from );
    Q_UNUSED( to );
    Q_UNUSED( flags );

    int* matchResult = static_cast<int*>( context );

    if ( id >= ExcludeMatchIdStart ) {
        *matchResult = 0;
        return 1;
    }
    else {
        ( *matchResult )++;
        return 0;
    }
}

} // namespace

HsMatcher::HsMatcher( HsDatabase db, HsScratch scratch, int requiredMatches )
    : database_{ std::move( db ) }
    , scratch_{ std::move( scratch ) }
    , requiredMatches_{ requiredMatches }
{
}

bool HsMatcher::hasMatch( const QString& data ) const
{
    if ( !scratch_ || !database_ ) {
        return false;
    }

    int matchResult = 0;

    const auto utf8Data = data.toUtf8();
    const auto scanResult
        = hs_scan( database_.get(), utf8Data.data(), static_cast<unsigned int>( utf8Data.size() ),
                   0, scratch_.get(), matchCallback, static_cast<void*>( &matchResult ) );

    return scanResult != HS_SCAN_TERMINATED && matchResult == requiredMatches_;
}

HsRegularExpression::HsRegularExpression( const RegularExpressionPattern& pattern )
    : HsRegularExpression( std::vector<RegularExpressionPattern>{ pattern } )
{
}

HsRegularExpression::HsRegularExpression( const std::vector<RegularExpressionPattern>& patterns )
    : pattern_( patterns.front() )
{
    requiredMatches_ = static_cast<int>( std::count_if(
        patterns.begin(), patterns.end(), []( const auto& p ) { return !p.isExclude; } ) );

    database_ = HsDatabase{ wrapHsPointer<hs_database_t, hs_free_database>(
        []( const std::vector<RegularExpressionPattern>& expressions,
            QString& errorMessage ) -> hs_database_t* {
            hs_database_t* db = nullptr;
            hs_compile_error_t* error = nullptr;

            std::vector<unsigned> flags;
            flags.reserve( expressions.size() );
            std::transform( expressions.begin(), expressions.end(), std::back_inserter( flags ),
                            []( const auto& expression ) {
                                auto expressionFlags
                                    = HS_FLAG_UTF8 | HS_FLAG_UCP | HS_FLAG_SINGLEMATCH;
                                if ( !expression.isCaseSensitive ) {
                                    expressionFlags |= HS_FLAG_CASELESS;
                                }
                                return expressionFlags;
                            } );

            std::vector<QByteArray> utf8Patterns;
            utf8Patterns.reserve( expressions.size() );
            std::transform( expressions.begin(), expressions.end(),
                            std::back_inserter( utf8Patterns ),
                            []( const auto& expression ) { return expression.pattern.toUtf8(); } );

            std::vector<const char*> patternPointers;
            utf8Patterns.reserve( utf8Patterns.size() );
            std::transform( utf8Patterns.begin(), utf8Patterns.end(),
                            std::back_inserter( patternPointers ),
                            []( const auto& utf8Pattern ) { return utf8Pattern.data(); } );

            std::vector<unsigned> expressionIds;
            expressionIds.resize( expressions.size() );
            for ( auto index = 0u; index < expressions.size(); ++index ) {
                expressionIds[ index ]
                    = index + ( expressions[ index ].isExclude ? ExcludeMatchIdStart : 0 );
            }

            const auto compileResult = hs_compile_multi(
                patternPointers.data(), flags.data(), expressionIds.data(),
                static_cast<unsigned>( expressions.size() ), HS_MODE_BLOCK, nullptr, &db, &error );

            if ( compileResult != HS_SUCCESS ) {
                LOG_ERROR << "Failed to compile pattern " << error->message;
                errorMessage = error->message;
                hs_free_compile_error( error );
                return nullptr;
            }

            return db;
        },
        patterns, errorMessage_ ) };

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

MatcherVariant HsRegularExpression::createMatcher() const
{
    if ( !isValid() ) {
        return MatcherVariant{ DefaultRegularExpressionMatcher( pattern_ ) };
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

    return MatcherVariant{ HsMatcher{ database_, std::move( matcherScratch ), requiredMatches_ } };
}
#endif

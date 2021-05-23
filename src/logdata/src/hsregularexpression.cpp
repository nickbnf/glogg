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

#include <algorithm>
#include <iterator>
#include <qregularexpression.h>
#include <string_view>
#include <vector>
#ifdef KLOGG_HAS_HS
#include "hsregularexpression.h"

#include "log.h"

namespace {

struct MatcherContext {
    const std::vector<std::string>& patternIds;
    std::vector<std::string>& matchingPatterns;
};

int matchCallback( unsigned int id, unsigned long long from, unsigned long long to,
                   unsigned int flags, void* context )
{
    Q_UNUSED( from );
    Q_UNUSED( to );
    Q_UNUSED( flags );

    MatcherContext* matchResult = static_cast<MatcherContext*>( context );

    const auto& patternId = matchResult->patternIds[ id ];
    const auto newMatch = std::find( matchResult->matchingPatterns.begin(),
                                     matchResult->matchingPatterns.end(), patternId )
                          == matchResult->matchingPatterns.end();

    if ( newMatch ) {
        matchResult->matchingPatterns.push_back( patternId );
    }
    return 0;
}

} // namespace

HsMatcher::HsMatcher( HsDatabase db, HsScratch scratch, const std::vector<std::string>& patternIds )
    : database_{ std::move( db ) }
    , scratch_{ std::move( scratch ) }
    , patternIds_{ patternIds }
{
}

std::unordered_map<std::string, bool> HsMatcher::match( const std::string_view& utf8Data ) const
{
    std::vector<std::string> matchingPatterns;

    if ( !scratch_ || !database_ ) {
        return {};
    }

    MatcherContext context{ patternIds_, matchingPatterns };

    hs_scan( database_.get(), utf8Data.data(), static_cast<unsigned int>( utf8Data.size() ), 0,
             scratch_.get(), matchCallback, static_cast<void*>( &context ) );

    std::unordered_map<std::string, bool> results;
    for ( const auto& patternId : patternIds_ ) {
        results[ patternId ] = false;
    }

    for ( const auto& match : matchingPatterns ) {
        results[ match ] = true;
    }

    return results;
}

HsRegularExpression::HsRegularExpression( const RegularExpressionPattern& pattern )
    : HsRegularExpression( std::vector<RegularExpressionPattern>{ pattern } )
{
}

HsRegularExpression::HsRegularExpression( const std::vector<RegularExpressionPattern>& patterns )
    : patterns_( patterns )
{
    std::transform( patterns.begin(), patterns.end(), std::back_inserter( patternIds_ ),
                    []( const auto& p ) { return p.id(); } );

    database_ = HsDatabase{ makeUniqueResource<hs_database_t, hs_free_database>(
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
                            std::back_inserter( utf8Patterns ), []( const auto& expression ) {
                                auto p = expression.pattern;
                                if ( expression.isPlainText ) {
                                    p = QRegularExpression::escape( expression.pattern );
                                }
                                return p.toUtf8();
                            } );

            std::vector<const char*> patternPointers;
            utf8Patterns.reserve( utf8Patterns.size() );
            std::transform( utf8Patterns.begin(), utf8Patterns.end(),
                            std::back_inserter( patternPointers ),
                            []( const auto& utf8Pattern ) { return utf8Pattern.data(); } );

            std::vector<unsigned> expressionIds;
            expressionIds.resize( expressions.size() );
            for ( auto index = 0u; index < expressions.size(); ++index ) {
                expressionIds[ index ] = index;
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
        scratch_ = makeUniqueResource<hs_scratch_t, hs_free_scratch>(
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

    if ( !isHsValid() ) {
        for ( const auto& pattern : patterns_ ) {
            const auto& regex = static_cast<QRegularExpression>( pattern );
            if ( !regex.isValid() ) {
                isValid_ = false;
                errorMessage_ = regex.errorString();
                break;
            }
        }
    }
}

bool HsRegularExpression::isValid() const
{
    return isValid_;
}

bool HsRegularExpression::isHsValid() const
{
    return database_ != nullptr && scratch_ != nullptr;
}

QString HsRegularExpression::errorString() const
{
    return errorMessage_;
}

MatcherVariant HsRegularExpression::createMatcher() const
{
    if ( !isHsValid() ) {
        return MatcherVariant{ DefaultRegularExpressionMatcher( patterns_ ) };
    }

    auto matcherScratch = makeUniqueResource<hs_scratch_t, hs_free_scratch>(
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

    return MatcherVariant{ HsMatcher{ database_, std::move( matcherScratch ), patternIds_ } };
}
#endif

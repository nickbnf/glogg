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

#include <exception>
#include <memory>
#include <unordered_map>
#include <variant>

#include <exprtk.hpp>
#include <vector>

#include "configuration.h"
#include "log.h"
#include "uuid.h"

#include "regularexpression.h"

namespace {
std::vector<RegularExpressionPattern>
parseBooleanExpressions( QString& pattern, bool isCaseSensitive, bool isPlainText )
{
    std::vector<RegularExpressionPattern> subPatterns;

    auto currentIndex = 0;
    auto leftQuote = -1;
    auto rightQuote = -1;

    while ( currentIndex < pattern.size() ) {
        leftQuote = pattern.indexOf( QChar( '"' ), currentIndex );
        if ( leftQuote < 0 ) {
            break;
        }

        currentIndex = leftQuote + 1;
        if ( leftQuote > 0 && pattern[ leftQuote - 1 ] == QChar( '\\' ) ) {
            leftQuote = -1;
            continue;
        }

        while ( currentIndex < pattern.size() ) {
            rightQuote = pattern.indexOf( QChar( '"' ), currentIndex );
            if ( rightQuote < 0 ) {
                break;
            }

            currentIndex = rightQuote + 1;
            if ( rightQuote > 0 && pattern[ rightQuote - 1 ] == QChar( '\\' ) ) {
                rightQuote = -1;
                continue;
            }

            break;
        }

        if ( rightQuote < 0 ) {
            break;
        }

        const auto subPatternLength = rightQuote - leftQuote - 1;
        auto subPattern = pattern.mid( leftQuote + 1, subPatternLength );
        subPattern.replace( "\\\"", "\"" );

        subPatterns.emplace_back( subPattern, isCaseSensitive, false, false, isPlainText );

        pattern.replace( leftQuote, subPatternLength + 2,
                         QString::fromStdString( subPatterns.back().id() ) );

        currentIndex = 0;
        leftQuote = -1;
        rightQuote = -1;
    }

    if ( pattern.contains( '"' ) ) {
        throw std::runtime_error( "Pattern has unmatched quotes" );
    }

    return subPatterns;
}

} // namespace

class BooleanExpressionEvaluator {
  public:
    BooleanExpressionEvaluator( const std::string& expression,
                                const std::vector<RegularExpressionPattern>& patterns )
    {
        for ( const auto& p : patterns ) {
            symbols_.create_variable( p.id() );
        }
        expression_.register_symbol_table( symbols_ );
        isValid_ = parser_.compile( expression, expression_ );
        if ( !isValid_ ) {
            errorString_ = parser_.error();
        }
    }

    bool isValid() const
    {
        return isValid_;
    }

    std::string errorString() const
    {
        return errorString_;
    }

    bool evaluate( const robin_hood::unordered_flat_map<std::string, bool>& variables )
    {
        if ( !isValid() ) {
            return false;
        }

        for ( const auto& result : variables ) {
            symbols_.get_variable( result.first )->ref() = result.second ? 1 : 0;
        }

        return expression_.value() > 0;
    }

  private:
    bool isValid_ = true;
    std::string errorString_;

    exprtk::symbol_table<double> symbols_;
    exprtk::expression<double> expression_;
    exprtk::parser<double> parser_;
};

RegularExpression::RegularExpression( const RegularExpressionPattern& pattern )
    : isInverse_( pattern.isExclude )
    , expression_( pattern.pattern )
{
    try {
        if ( pattern.isBoolean ) {
            subPatterns_ = parseBooleanExpressions( expression_, pattern.isCaseSensitive,
                                                    pattern.isPlainText );

            BooleanExpressionEvaluator evaluator{ expression_.toStdString(), subPatterns_ };
            if ( !evaluator.isValid() ) {
                isValid_ = false;
                errorString_ = QString::fromStdString( evaluator.errorString() );
                return;
            }
        }
        else {
            subPatterns_.emplace_back( pattern );
            expression_ = QString::fromStdString( subPatterns_.front().id() );
        }

        hsExpression_ = HsRegularExpression( subPatterns_ );
        isValid_ = hsExpression_.isValid();
        errorString_ = hsExpression_.errorString();

    } catch ( std::exception& err ) {
        isValid_ = false;
        errorString_ = err.what();
    }
}

bool RegularExpression::isValid() const
{
    return isValid_;
}

QString RegularExpression::errorString() const
{
    return errorString_;
}

std::unique_ptr<PatternMatcher> RegularExpression::createMatcher() const
{
    return std::make_unique<PatternMatcher>( *this );
}

PatternMatcher::PatternMatcher( const RegularExpression& expression )
    : isInverse_( expression.isInverse_ )
    , matcher_( expression.hsExpression_.createMatcher() )
    , evaluator_( std::make_unique<BooleanExpressionEvaluator>(
          expression.expression_.toStdString(), expression.subPatterns_ ) )
{
    const auto& config = Configuration::get();
    const auto useHyperscanEngine = config.regexpEngine() == RegexpEngine::Hyperscan;
    if ( !useHyperscanEngine ) {
        matcher_ = DefaultRegularExpressionMatcher( expression.subPatterns_ );
    }
}

PatternMatcher::~PatternMatcher() = default;

bool PatternMatcher::hasMatch( std::string_view line ) const
{
    const auto isMatch = hasMatchInternal( line );
    return ( !isInverse_ ) ? isMatch : !isMatch;
}

bool PatternMatcher::hasMatchInternal( std::string_view line ) const
{
    const auto results
        = std::visit( [ &line ]( const auto& m ) { return m.match( line ); }, matcher_ );

    if ( results.size() == 1 ) {
        return results.begin()->second;
    }

    return evaluator_->evaluate( results );
}
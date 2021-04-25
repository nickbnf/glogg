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

#ifndef KLOGG_HS_REGULAR_EXPRESSION
#define KLOGG_HS_REGULAR_EXPRESSION

#include <QRegularExpression>
#include <QString>

#ifdef KLOGG_HAS_HS
#include <hs.h>
#include <memory>
#endif

struct RegularExpressionPattern {
    QString pattern;
    bool isCaseSensitive = true;
    bool isExclude = false;

    RegularExpressionPattern() = default;

    explicit RegularExpressionPattern( const QString& expression )
        : pattern( expression )
    {
    }

    explicit RegularExpressionPattern( const QRegularExpression& regexp )
        : pattern( regexp.pattern() )
        , isCaseSensitive(
              !regexp.patternOptions().testFlag( QRegularExpression::CaseInsensitiveOption ) )
    {
    }

    explicit operator QRegularExpression() const
    {
        auto patternOptions = QRegularExpression::UseUnicodePropertiesOption
                              | QRegularExpression::DontCaptureOption;

        if ( !isCaseSensitive ) {
            patternOptions |= QRegularExpression::CaseInsensitiveOption;
        }

        return QRegularExpression( pattern, patternOptions );
    }

    bool operator==( const RegularExpressionPattern& other ) const
    {
        return std::tie( pattern, isCaseSensitive, isExclude )
               == std::tie( other.pattern, other.isCaseSensitive, other.isExclude );
    }
};

class DefaultRegularExpressionMatcher {
  public:
    explicit DefaultRegularExpressionMatcher( const RegularExpressionPattern& regexp )
        : regexp_( static_cast<QRegularExpression>( regexp ) )
        , isExclude_( regexp.isExclude )
    {
    }

    DefaultRegularExpressionMatcher( const QRegularExpression& regexp, bool isExclude )
        : regexp_( regexp.pattern(), regexp.patternOptions() )
        , isExclude_( isExclude )
    {
    }

    bool hasMatch( const QString& data ) const
    {
        const auto isMatching = regexp_.match( data ).hasMatch();
        return ( !isExclude_ && isMatching ) || ( isExclude_ && !isMatching );
    }

  private:
    QRegularExpression regexp_;
    bool isExclude_ = false;
};

#ifdef KLOGG_HAS_HS

template <typename T, int ( *Func )( T* )>
struct HsDeleteHelper {
    void operator()( T* t )
    {
        Func( t );
    }
};

template <typename T, int ( *Deleter )( T* ), typename CreateFunc, typename... Args>
std::unique_ptr<T, HsDeleteHelper<T, Deleter>> wrapHsPointer( CreateFunc createFunc,
                                                              Args&&... args )
{
    return { createFunc( std::forward<Args>( args )... ), {} };
}

using HsScratchDeleter = HsDeleteHelper<hs_scratch_t, hs_free_scratch>;
using HsScratch = std::unique_ptr<hs_scratch_t, HsScratchDeleter>;

using HsDatabaseDeleter = HsDeleteHelper<hs_database_t, hs_free_database>;
using HsDatabase = std::shared_ptr<hs_database_t>;

class HsMatcher {
  public:
    HsMatcher() = default;
    HsMatcher( HsDatabase database, HsScratch scratch, int requiredMatches );

    HsMatcher( const HsMatcher& ) = delete;
    HsMatcher& operator=( const HsMatcher& ) = delete;

    HsMatcher( HsMatcher&& other ) = default;
    HsMatcher& operator=( HsMatcher&& other ) = default;

    bool hasMatch( const QString& data ) const;

  private:
    HsDatabase database_;
    HsScratch scratch_;
    int requiredMatches_ = 1;
};

class HsRegularExpression {
  public:
    explicit HsRegularExpression( const RegularExpressionPattern& includePattern );
    explicit HsRegularExpression( const std::vector<RegularExpressionPattern>& patterns );

    HsRegularExpression( const HsRegularExpression& ) = delete;
    HsRegularExpression& operator=( const HsRegularExpression& ) = delete;

    HsRegularExpression( HsRegularExpression&& other ) = default;
    HsRegularExpression& operator=( HsRegularExpression&& other ) = default;

    bool isValid() const;
    QString errorString() const;

    HsMatcher createMatcher() const;

  private:
    HsDatabase database_;
    HsScratch scratch_;
    int requiredMatches_ = 1;

    QString errorMessage_;
};
#else
class HsMatcher : public DefaultRegularExpressionMatcher {
  public:
    explicit HsMatcher( const QRegularExpression& regexp, bool isExclude )
        : DefaultRegularExpressionMatcher( regexp, isExclude )
    {
    }
};

class HsRegularExpression {
  public:
    explicit HsRegularExpression( const RegularExpressionPattern& regexp )
        : regexp_( static_cast<QRegularExpression>( regexp ) )
        , isExclude_( regexp.isExclude )
    {
    }

    bool isValid() const
    {
        return regexp_.isValid();
    }

    QString errorString() const
    {
        return regexp_.errorString();
    }

    HsMatcher createMatcher() const
    {
        return HsMatcher( regexp_, isExclude_ );
    }

  private:
    QRegularExpression regexp_;
    bool isExclude_ = false;
};

#endif

#endif
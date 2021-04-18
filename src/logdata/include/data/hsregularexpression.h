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
#endif

class DefaultRegularExpressionMatcher {
  public:
    explicit DefaultRegularExpressionMatcher( const QRegularExpression& regexp )
        : regexp_( regexp.pattern(), regexp.patternOptions() )
    {
    }

    bool hasMatch( const QString& data ) const
    {
        return regexp_.match( data ).hasMatch();
    }

  private:
    QRegularExpression regexp_;
};

#ifdef KLOGG_HAS_HS
class HsMatcher {
  public:
    HsMatcher() = default;
    HsMatcher( const hs_database_t* db, hs_scratch_t* scratch );
    ~HsMatcher();

    HsMatcher( const HsMatcher& ) = delete;
    HsMatcher& operator=( const HsMatcher& ) = delete;

    HsMatcher( HsMatcher&& other );
    HsMatcher& operator=( HsMatcher&& other );

    bool hasMatch( const QString& data ) const;

  private:
    const hs_database_t* database_;
    hs_scratch_t* scratch_ = nullptr;
};

class HsRegularExpression {
  public:
    explicit HsRegularExpression( const QRegularExpression& pattern );
    ~HsRegularExpression();

    HsRegularExpression( const HsRegularExpression& ) = delete;
    HsRegularExpression& operator=( const HsRegularExpression& ) = delete;

    HsRegularExpression( HsRegularExpression&& other );
    HsRegularExpression& operator=( HsRegularExpression&& other );

    bool isValid() const;
    QString errorString() const;

    HsMatcher createMatcher() const;

  private:
    hs_database_t* database_ = nullptr;
    hs_scratch_t* scratch_ = nullptr;

    QString errorMessage_;
};
#else
class HsMatcher : public DefaultRegularExpressionMatcher {
  public:
    explicit HsMatcher( const QRegularExpression& regexp )
        : DefaultRegularExpressionMatcher( regexp )
    {
    }
};

class HsRegularExpression {
  public:
    explicit HsRegularExpression( const QRegularExpression& regexp )
        : regexp_( regexp )
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
        return HsMatcher( regexp_ );
    }

  private:
    QRegularExpression regexp_;
};

#endif

#endif
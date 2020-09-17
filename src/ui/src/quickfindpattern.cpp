/*
 * Copyright (C) 2010 Nicolas Bonnefon and other contributors
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

// This file implements QuickFindPattern.
// This class implements part of the Quick Find mechanism, it only stores the
// current search pattern, once it has been confirmed (return pressed),
// it can be asked to return the matches in a specific string.

#include <iostream>

#include "quickfindpattern.h"

#include "configuration.h"

#include "quickfind.h"

constexpr Qt::GlobalColor QfForeColor = Qt::black;
constexpr Qt::GlobalColor QfBackColor = Qt::yellow;

bool QuickFindMatcher::isLineMatching( const QString& line, int column ) const
{
    if ( !isActive_ )
        return false;

    QRegularExpressionMatch match = regexp_.match( line, column );
    if ( match.hasMatch() ) {
        lastMatchStart_ = match.capturedStart();
        lastMatchEnd_ = match.capturedEnd() - 1;
        return true;
    }
    else {
        return false;
    }
}

bool QuickFindMatcher::isLineMatchingBackward( const QString& line, int column ) const
{
    if ( !isActive_ )
        return false;

    QRegularExpressionMatchIterator matches = regexp_.globalMatch( line );
    QRegularExpressionMatch lastMatch;
    while ( matches.hasNext() ) {
        QRegularExpressionMatch nextMatch = matches.peekNext();
        if ( column >= 0 && nextMatch.capturedEnd() >= column ) {
            break;
        }

        lastMatch = matches.next();
    }

    if ( lastMatch.hasMatch() ) {
        lastMatchStart_ = lastMatch.capturedStart();
        lastMatchEnd_ = lastMatch.capturedEnd() - 1;
        return true;
    }
    else {
        return false;
    }
}

void QuickFindMatcher::getLastMatch( int* start_col, int* end_col ) const
{
    *start_col = lastMatchStart_;
    *end_col = lastMatchEnd_;
}

void QuickFindPattern::changeSearchPattern( const QString& pattern )
{
    pattern_ = pattern;

    // Determine the type of regexp depending on the config
    QString searchPattern;
    switch ( Configuration::get().quickfindRegexpType() ) {
    case Wildcard:
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 12, 0 ) )
        searchPattern = QRegularExpression::wildcardToRegularExpression( pattern );
        searchPattern = searchPattern.mid( 2, searchPattern.size() - 4 );
#else
        searchPattern = pattern;
        searchPattern.replace( '*', ".*" ).replace( '?', "." );
#endif

        break;
    case FixedString:
        searchPattern = QRegularExpression::escape( pattern );
        break;
    default:
        searchPattern = pattern;
        break;
    }

    regexp_.setPattern( searchPattern );

    if ( regexp_.isValid() && ( !searchPattern.isEmpty() ) )
        active_ = true;
    else
        active_ = false;

    emit patternUpdated();
}

void QuickFindPattern::changeSearchPattern( const QString& pattern, bool ignoreCase )
{
    QRegularExpression::PatternOptions options = QRegularExpression::UseUnicodePropertiesOption;

    if ( ignoreCase )
        options |= QRegularExpression::CaseInsensitiveOption;

    regexp_.setPatternOptions( options );
    changeSearchPattern( pattern );
}

bool QuickFindPattern::matchLine( const QString& line, std::vector<HighlightedMatch>& matches ) const
{
    matches.clear();

    if ( active_ ) {
        QRegularExpressionMatchIterator matchIterator = regexp_.globalMatch( line );

        while ( matchIterator.hasNext() ) {
            QRegularExpressionMatch match = matchIterator.next();
            matches.emplace_back( match.capturedStart(), match.capturedLength(), QfForeColor, QfBackColor );
        }
    }

    return ( !matches.empty() );
}

QuickFindMatcher QuickFindPattern::getMatcher() const
{
    return QuickFindMatcher( active_, regexp_ );
}

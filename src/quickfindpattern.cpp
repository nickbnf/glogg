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

// This file implements QuickFindPattern.
// This class implements part of the Quick Find mechanism, it only stores the
// current search pattern, once it has been confirmed (return pressed),
// it can be asked to return the matches in a specific string.

#include "quickfindpattern.h"

#include "persistentinfo.h"
#include "configuration.h"

QuickFindPattern::QuickFindPattern() : QObject(), regexp_()
{
    active_ = false;
}

void QuickFindPattern::changeSearchPattern( const QString& pattern )
{
    // Determine the type of regexp depending on the config
    QString searchPattern;
    switch ( Persistent<Configuration>( "settings" )->quickfindRegexpType() ) {
        case Wildcard:
            searchPattern = pattern;
            searchPattern.replace('*', ".*").replace('?', ".");
            break;
        case FixedString:
            searchPattern = QRegularExpression::escape(pattern);
            break;
        default:
            searchPattern = pattern;
            break;
    }

    regexp_.setPattern( pattern );

    if ( regexp_.isValid() && ( ! searchPattern.isEmpty() ) )
        active_ = true;
    else
        active_ = false;

    emit patternUpdated();
}

void QuickFindPattern::changeSearchPattern( const QString& pattern, bool ignoreCase )
{
    QRegularExpression::PatternOptions options =
            QRegularExpression::UseUnicodePropertiesOption;
#if QT_VERSION >= 0x050400
    options |= QRegularExpression::OptimizeOnFirstUsageOption;
#endif

    if ( ignoreCase )
        options |= QRegularExpression::CaseInsensitiveOption;

    regexp_.setPatternOptions(options);
    changeSearchPattern( pattern );
}

bool QuickFindPattern::matchLine( const QString& line,
        QList<QuickFindMatch>& matches ) const
{
    matches.clear();

    if ( active_ ) {
        QRegularExpressionMatchIterator matchIterator = regexp_.globalMatch(line);

        while( matchIterator.hasNext() ) {
            QRegularExpressionMatch match = matchIterator.next();
            matches << QuickFindMatch ( match.capturedStart(), match.capturedLength() );
        }
    }

    return ( matches.count() > 0 );
}

bool QuickFindPattern::isLineMatching( const QString& line, int column ) const
{
    if ( ! active_ )
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

bool QuickFindPattern::isLineMatchingBackward(
        const QString& line, int column ) const
{
    int pos = 0;

    if ( ! active_ )
        return false;

    QRegularExpressionMatchIterator matches = regexp_.globalMatch(line);
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

void QuickFindPattern::getLastMatch( int* start_col, int* end_col ) const
{
    *start_col = lastMatchStart_;
    *end_col   = lastMatchEnd_;
}

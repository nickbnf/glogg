/*
 * Copyright (C) 2010, 2013 Nicolas Bonnefon and other contributors
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

#ifndef QUICKFINDPATTERN_H
#define QUICKFINDPATTERN_H

#include <QObject>
#include <QString>
#include <QRegularExpression>
#include <QList>

// Represents a match result for QuickFind
class QuickFindMatch
{
  public:
    // Construct a match (must be initialised)
    QuickFindMatch( int start_column, int length )
    { startColumn_ = start_column; length_ = length; }

    // Accessor functions
    int startColumn() const { return startColumn_; }
    int length() const { return length_; }

  private:
    int startColumn_;
    int length_;
};

// Represents a search pattern for QuickFind (without its results)
class QuickFindPattern : public QObject
{
  Q_OBJECT

  public:
    // Construct an empty search
    QuickFindPattern();

    // Set the search to a new pattern, using the current
    // case status
    void changeSearchPattern( const QString& pattern );

    // Set the search to a new pattern, as well as the case status
    void changeSearchPattern( const QString& pattern, bool ignoreCase );

    // Returns whether the search is active (i.e. valid and non empty regexp)
    bool isActive() const { return active_; }

    // Return the text of the regex
    QString getPattern() const { return regexp_.pattern(); }

    // Returns whether the passed line match the quick find search.
    // If so, it populate the passed list with the list of matches
    // within this particular line.
    bool matchLine( const QString& line,
            QList<QuickFindMatch>& matches ) const;

    // Returns whether there is a match in the passed line, starting at
    // the passed column.
    // Results are stored internally.
    bool isLineMatching( const QString& line, int column = 0 ) const;

    // Same as isLineMatching but search backward
    bool isLineMatchingBackward( const QString& line, int column = -1 ) const;

    // Must be called when isLineMatching returns 'true', returns
    // the position of the first match found.
    void getLastMatch( int* start_col, int* end_col ) const;

  signals:
    // Sent when the pattern is changed
    void patternUpdated();

  private:
    bool active_;
    QRegularExpression regexp_;

    mutable int lastMatchStart_;
    mutable int lastMatchEnd_;
};

#endif

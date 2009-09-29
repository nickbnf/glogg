/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FILTERSET_H
#define FILTERSET_H

#include <QRegexp>
#include <QColor>

// Represents a filter, i.e. a regexp and the colors matching text
// should be rendered in.
class Filter
{
  public:
    Filter( const QString& pattern,
            const QColor& foreColor, const QColor& backColor );

    QString pattern() const;
    void setPattern( const QString& pattern );
    const QColor& foreColor() const;
    void setForeColor( const QColor& foreColor );
    const QColor& backColor() const;
    void setBackColor( const QColor& backColor );

  private:
    QRegExp regexp_;
    QColor foreColor_;
    QColor backColor_;
    bool enabled_;
};

// Represents an ordered set of filters to be applied to each line displayed.
class FilterSet
{
  public:
    // Construct an empty filter set
    FilterSet();

    // Returns weither the passed line match a filter of the set,
    // if so, it returns the fore/back colors the line should use.
    // Ownership of the colors is transfered to the caller.
    bool matchLine( const QString& line,
            QColor* foreColor, QColor* backColor );

  private:
    QList<Filter> filterList;

    // To simplify this class interface, FilterDialog can access our
    // internal structure directly.
    friend class FiltersDialog;
};

#endif

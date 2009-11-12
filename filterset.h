/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
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

#ifndef FILTERSET_H
#define FILTERSET_H

#include <QRegExp>
#include <QColor>
#include <QMetaType>

// Represents a filter, i.e. a regexp and the colors matching text
// should be rendered in.
class Filter
{
  public:
    // Construct an uninitialized Filter (when reading from a config file)
    Filter();
    Filter( const QString& pattern,
            const QString& foreColor, const QString& backColor );

    // Tests the string passed for a match, returns a value just like
    // QRegExp::indexIn (i.e. -1 if no match)
    int indexIn( const QString& string ) const;

    // Accessor functions
    QString pattern() const;
    void setPattern( const QString& pattern );
    const QString& foreColorName() const;
    void setForeColor( const QString& foreColorName );
    const QString& backColorName() const;
    void setBackColor( const QString& backColorName );

    // Operators for serialization
    friend QDataStream& operator<<( QDataStream& out, const Filter& object );
    friend QDataStream& operator>>( QDataStream& in, Filter& object );

  private:
    QRegExp regexp_;
    QString foreColorName_;
    QString backColorName_;
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
            QColor* foreColor, QColor* backColor ) const;

    // Operators for serialization
    friend QDataStream& operator<<(
            QDataStream& out, const FilterSet& object );
    friend QDataStream& operator>>(
            QDataStream& in, FilterSet& object );

  private:
    QList<Filter> filterList;

    // To simplify this class interface, FilterDialog can access our
    // internal structure directly.
    friend class FiltersDialog;
};

Q_DECLARE_METATYPE(FilterSet)

#endif

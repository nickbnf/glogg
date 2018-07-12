/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

#ifndef HIGHLIGHTERSET_H
#define HIGHLIGHTERSET_H

#include <QRegularExpression>
#include <QColor>
#include <QMetaType>

#include "persistable.h"

// Represents a highlighter, i.e. a regexp and the colors matching text
// should be rendered in.
class Highlighter
{
  public:
    // Construct an uninitialized Highlighter (when reading from a config file)
    Highlighter();
    Highlighter(const QString& pattern, bool ignoreCase,
            const QString& foreColor, const QString& backColor );

    bool hasMatch( const QString& string ) const;

    // Accessor functions
    QString pattern() const;
    void setPattern( const QString& pattern );
    bool ignoreCase() const;
    void setIgnoreCase( bool ignoreCase );
    const QString& foreColorName() const;
    void setForeColor( const QString& foreColorName );
    const QString& backColorName() const;
    void setBackColor( const QString& backColorName );

    // Operators for serialization
    // (must be kept to migrate highlighters from <=0.8.2)
    friend QDataStream& operator<<( QDataStream& out, const Highlighter& object );
    friend QDataStream& operator>>( QDataStream& in, Highlighter& object );

    // Reads/writes the current config in the QSettings object passed
    void saveToStorage( QSettings& settings ) const;
    void retrieveFromStorage( QSettings& settings );

  private:
    QRegularExpression regexp_;
    QString foreColorName_;
    QString backColorName_;
    bool enabled_;
};

// Represents an ordered set of highlighters to be applied to each line displayed.
class HighlighterSet : public Persistable
{
  public:
    // Construct an empty highlighter set
    HighlighterSet();

    // Returns weither the passed line match a highlighter of the set,
    // if so, it returns the fore/back colors the line should use.
    // Ownership of the colors is transfered to the caller.
    bool matchLine( const QString& line,
            QColor* foreColor, QColor* backColor ) const;

    // Reads/writes the current config in the QSettings object passed
    virtual void saveToStorage( QSettings& settings ) const;
    virtual void retrieveFromStorage( QSettings& settings );

    // Should be private really, but I don't know how to have 
    // it recognised by QVariant then.
    typedef QList<Highlighter> HighlighterList;

    // Operators for serialization
    // (must be kept to migrate highlighters from <=0.8.2)
    friend QDataStream& operator<<(
            QDataStream& out, const HighlighterSet& object );
    friend QDataStream& operator>>(
            QDataStream& in, HighlighterSet& object );

  private:
    static const int HIGHLIGHTERSET_VERSION;

    HighlighterList highlighterList;

    // To simplify this class interface, HighlighterDialog can access our
    // internal structure directly.
    friend class HighlightersDialog;
};

Q_DECLARE_METATYPE(Highlighter)
Q_DECLARE_METATYPE(HighlighterSet)
Q_DECLARE_METATYPE(HighlighterSet::HighlighterList)

#endif

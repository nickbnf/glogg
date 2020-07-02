/*
 * Copyright (C) 2009, 2010, 2011 Nicolas Bonnefon and other contributors
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
 * Copyright (C) 2019 Anton Filimonov and other contributors
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

// This file implements classes Highlighter and HighlighterSet

#include <QDataStream>
#include <QSettings>

#include "highlighterset.h"
#include "log.h"

QRegularExpression::PatternOptions getPatternOptions( bool ignoreCase )
{
    QRegularExpression::PatternOptions options = QRegularExpression::UseUnicodePropertiesOption;

    if ( ignoreCase ) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }
    return options;
}

Highlighter::Highlighter( const QString& pattern, bool ignoreCase, const QColor& foreColor,
                          const QColor& backColor )
    : regexp_( pattern, getPatternOptions( ignoreCase ) )
    , foreColor_( foreColor )
    , backColor_( backColor )
{
    LOG( logDEBUG ) << "New Highlighter, fore: " << foreColor_.name()
                    << " back: " << backColor_.name();
}

QString Highlighter::pattern() const
{
    return regexp_.pattern();
}

void Highlighter::setPattern( const QString& pattern )
{
    regexp_.setPattern( pattern );
}

bool Highlighter::ignoreCase() const
{
    return regexp_.patternOptions().testFlag( QRegularExpression::CaseInsensitiveOption );
}

void Highlighter::setIgnoreCase( bool ignoreCase )
{
    regexp_.setPatternOptions( getPatternOptions( ignoreCase ) );
}

const QColor& Highlighter::foreColor() const
{
    return foreColor_;
}

void Highlighter::setForeColor( const QColor& foreColor )
{
    foreColor_ = foreColor;
}

const QColor& Highlighter::backColor() const
{
    return backColor_;
}

void Highlighter::setBackColor( const QColor& backColor )
{
    backColor_ = backColor;
}

bool Highlighter::hasMatch( const QString& string ) const
{
    return regexp_.match( string ).hasMatch();
}

//
// Operators for serialization
//

QDataStream& operator<<( QDataStream& out, const Highlighter& object )
{
    LOG( logDEBUG ) << "<<operator from Highlighter";
    out << object.regexp_;
    out << object.foreColor_;
    out << object.backColor_;

    return out;
}

QDataStream& operator>>( QDataStream& in, Highlighter& object )
{
    LOG( logDEBUG ) << ">>operator from Highlighter";
    in >> object.regexp_;
    in >> object.foreColor_;
    in >> object.backColor_;

    return in;
}

// Default constructor
HighlighterSet::HighlighterSet()
{
    qRegisterMetaTypeStreamOperators<Highlighter>( "Highlighter" );
    qRegisterMetaTypeStreamOperators<HighlighterSet>( "HighlighterSet" );
    qRegisterMetaTypeStreamOperators<HighlighterSet::HighlighterList>(
        "HighlighterSet::HighlighterList" );
}

bool HighlighterSet::matchLine( const QString& line, QColor* foreColor, QColor* backColor ) const
{
    for ( auto i = highlighterList_.constBegin(); i != highlighterList_.constEnd(); ++i ) {
        if ( i->hasMatch( line ) ) {
            *foreColor = i->foreColor();
            *backColor = i->backColor();
            return true;
        }
    }

    return false;
}

//
// Operators for serialization
//

QDataStream& operator<<( QDataStream& out, const HighlighterSet& object )
{
    LOG( logDEBUG ) << "<<operator from HighlighterSet";
    out << object.highlighterList_;

    return out;
}

QDataStream& operator>>( QDataStream& in, HighlighterSet& object )
{
    LOG( logDEBUG ) << ">>operator from HighlighterSet";
    in >> object.highlighterList_;

    return in;
}

//
// Persistable virtual functions implementation
//

void Highlighter::saveToStorage( QSettings& settings ) const
{
    LOG( logDEBUG ) << "Highlighter::saveToStorage";

    settings.setValue( "regexp", regexp_.pattern() );
    settings.setValue( "ignore_case", regexp_.patternOptions().testFlag(
                                          QRegularExpression::CaseInsensitiveOption ) );
    // save colors as user friendly strings in config
    settings.setValue( "fore_colour", foreColor_.name() );
    settings.setValue( "back_colour", backColor_.name() );
}

void Highlighter::retrieveFromStorage( QSettings& settings )
{
    LOG( logDEBUG ) << "Highlighter::retrieveFromStorage";

    regexp_ = QRegularExpression(
        settings.value( "regexp" ).toString(),
        getPatternOptions( settings.value( "ignore_case", false ).toBool() ) );
    foreColor_ = QColor( settings.value( "fore_colour" ).toString() );
    backColor_ = QColor( settings.value( "back_colour" ).toString() );
}

void HighlighterSet::saveToStorage( QSettings& settings ) const
{
    LOG( logDEBUG ) << "HighlighterSet::saveToStorage";

    settings.beginGroup( "HighlighterSet" );
    settings.setValue( "version", HighlighterSet_VERSION );
    settings.remove( "highlighterss" );
    settings.beginWriteArray( "highlighters" );
    for ( int i = 0; i < highlighterList_.size(); ++i ) {
        settings.setArrayIndex( i );
        highlighterList_[ i ].saveToStorage( settings );
    }
    settings.endArray();
    settings.endGroup();
}

void HighlighterSet::retrieveFromStorage( QSettings& settings )
{
    LOG( logDEBUG ) << "HighlighterSet::retrieveFromStorage";

    highlighterList_.clear();

    if ( settings.contains( "FilterSet/version" ) ) {
        LOG( logINFO ) << "HighlighterSet found old filters";
        settings.beginGroup( "FilterSet" );
        if ( settings.value( "version" ).toInt() <= FilterSet_VERSION ) {
            int size = settings.beginReadArray( "filters" );
            for ( int i = 0; i < size; ++i ) {
                settings.setArrayIndex( i );
                Highlighter highlighter;
                highlighter.retrieveFromStorage( settings );
                highlighterList_.append( highlighter );
            }
            settings.endArray();
        }
        else {
            LOG( logERROR ) << "Unknown version of filterSet, ignoring it...";
        }
        settings.endGroup();
        settings.remove( "FilterSet" );

        saveToStorage( settings );
        settings.sync();
    }
    else if ( settings.contains( "HighlighterSet/version" ) ) {
        settings.beginGroup( "HighlighterSet" );
        if ( settings.value( "version" ).toInt() <= HighlighterSet_VERSION ) {
            int size = settings.beginReadArray( "highlighters" );
            for ( int i = 0; i < size; ++i ) {
                settings.setArrayIndex( i );
                Highlighter highlighter;
                highlighter.retrieveFromStorage( settings );
                highlighterList_.append( highlighter );
            }
            settings.endArray();
        }
        else {
            LOG( logERROR ) << "Unknown version of highlighterSet, ignoring it...";
        }
        settings.endGroup();
    }
    else {
        LOG( logWARNING ) << "Trying to import legacy (<=0.8.2) filters...";
        HighlighterSet tmp_highlighter_set = settings.value( "filterSet" ).value<HighlighterSet>();
        *this = tmp_highlighter_set;
        LOG( logWARNING ) << "...imported filterSet: " << highlighterList_.count() << " elements";
        // Remove the old key once migration is done
        settings.remove( "filterSet" );
        // And replace it with the new one
        saveToStorage( settings );
        settings.sync();
    }
}

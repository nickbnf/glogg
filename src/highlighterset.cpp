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

// This file implements classes Highlighter and HighlighterSet

#include <QSettings>
#include <QDataStream>

#include "log.h"
#include "highlighterset.h"

const int HighlighterSet::HIGHLIGHTERSET_VERSION = 1;

QRegularExpression::PatternOptions getPatternOptions( bool ignoreCase )
{
    QRegularExpression::PatternOptions options =
            QRegularExpression::UseUnicodePropertiesOption
            | QRegularExpression::OptimizeOnFirstUsageOption;

    if ( ignoreCase ) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }
    return options;
}

Highlighter::Highlighter()
{
}

Highlighter::Highlighter(const QString& pattern, bool ignoreCase,
            const QString& foreColorName, const QString& backColorName ) :
    regexp_( pattern,  getPatternOptions( ignoreCase ) ),
    foreColorName_( foreColorName ),
    backColorName_( backColorName ), enabled_( true )
{
    LOG(logDEBUG) << "New Highlighter, fore: " << foreColorName_.toStdString()
        << " back: " << backColorName_.toStdString();
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
    return regexp_.patternOptions().testFlag(QRegularExpression::CaseInsensitiveOption);
}

void Highlighter::setIgnoreCase( bool ignoreCase )
{
    regexp_.setPatternOptions( getPatternOptions( ignoreCase ) );
}

const QString& Highlighter::foreColorName() const
{
    return foreColorName_;
}

void Highlighter::setForeColor( const QString& foreColorName )
{
    foreColorName_ = foreColorName;
}

const QString& Highlighter::backColorName() const
{
    return backColorName_;
}

void Highlighter::setBackColor( const QString& backColorName )
{
    backColorName_ = backColorName;
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
    LOG(logDEBUG) << "<<operator from Highlighter";
    out << object.regexp_;
    out << object.foreColorName_;
    out << object.backColorName_;

    return out;
}

QDataStream& operator>>( QDataStream& in, Highlighter& object )
{
    LOG(logDEBUG) << ">>operator from Highlighter";
    in >> object.regexp_;
    in >> object.foreColorName_;
    in >> object.backColorName_;

    return in;
}


// Default constructor
HighlighterSet::HighlighterSet()
{
    qRegisterMetaTypeStreamOperators<Highlighter>( "Highlighter" );
    qRegisterMetaTypeStreamOperators<HighlighterSet>( "HighlighterSet" );
    qRegisterMetaTypeStreamOperators<HighlighterSet::HighlighterList>( "HighlighterSet::HighlighterList" );
}

bool HighlighterSet::matchLine( const QString& line,
        QColor* foreColor, QColor* backColor ) const
{
    for ( QList<Highlighter>::const_iterator i = highlighterList.constBegin();
          i != highlighterList.constEnd(); i++ ) {
        if ( i->hasMatch( line ) ) {
            foreColor->setNamedColor( i->foreColorName() );
            backColor->setNamedColor( i->backColorName() );
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
    LOG(logDEBUG) << "<<operator from HighlighterSet";
    out << object.highlighterList;

    return out;
}

QDataStream& operator>>( QDataStream& in, HighlighterSet& object )
{
    LOG(logDEBUG) << ">>operator from HighlighterSet";
    in >> object.highlighterList;

    return in;
}

//
// Persistable virtual functions implementation
//

void Highlighter::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "Highlighter::saveToStorage";

    settings.setValue( "regexp", regexp_.pattern() );
    settings.setValue( "ignore_case", regexp_.patternOptions().testFlag( QRegularExpression::CaseInsensitiveOption ) );
    settings.setValue( "fore_colour", foreColorName_ );
    settings.setValue( "back_colour", backColorName_ );
}

void Highlighter::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "Highlighter::retrieveFromStorage";

    regexp_ = QRegularExpression( settings.value( "regexp" ).toString(),
                       getPatternOptions( settings.value( "ignore_case", false ).toBool() ) );
    foreColorName_ = settings.value( "fore_colour" ).toString();
    backColorName_ = settings.value( "back_colour" ).toString();
}

void HighlighterSet::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "HighlighterSet::saveToStorage";

    settings.beginGroup( "HighlighterSet" );
    // Remove everything in case the array is shorter than the previous one
    settings.remove("");
    settings.setValue( "version", HIGHLIGHTERSET_VERSION );
    settings.beginWriteArray( "highlighters" );
    for (int i = 0; i < highlighterList.size(); ++i) {
        settings.setArrayIndex(i);
        highlighterList[i].saveToStorage( settings );
    }
    settings.endArray();
    settings.endGroup();
}

void HighlighterSet::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "HighlighterSet::retrieveFromStorage";

    highlighterList.clear();

    if ( settings.contains( "HighlighterSet/version" ) ) {
        settings.beginGroup( "HighlighterSet" );
        if ( settings.value( "version" ) == HIGHLIGHTERSET_VERSION ) {
            int size = settings.beginReadArray( "highlighters" );
            for (int i = 0; i < size; ++i) {
                settings.setArrayIndex(i);
                Highlighter highlighter;
                highlighter.retrieveFromStorage( settings );
                highlighterList.append( highlighter );
            }
            settings.endArray();
        }
        else {
            LOG(logERROR) << "Unknown version of HighlighterSet, ignoring it...";
        }
        settings.endGroup();
    }
    else {
        LOG(logWARNING) << "Trying to import legacy (<=0.8.2) highlighters...";
        HighlighterSet tmp_highlighter_set =
            settings.value( "highlighterSet" ).value<HighlighterSet>();
        *this = tmp_highlighter_set;
        LOG(logWARNING) << "...imported highlighterset: "
            << highlighterList.count() << " elements";
        // Remove the old key once migration is done
        settings.remove( "highlighterSet" );
        // And replace it with the new one
        saveToStorage( settings );
        settings.sync();
    }
}

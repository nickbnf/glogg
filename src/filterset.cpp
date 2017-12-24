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

// This file implements classes Filter and FilterSet

#include <QSettings>
#include <QDataStream>

#include "log.h"
#include "filterset.h"

const int FilterSet::FILTERSET_VERSION = 2;

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

Filter::Filter()
{
}

Filter::Filter(const QString& pattern, bool ignoreCase,
            const QColor& foreColor, const QColor& backColor ) :
    regexp_( pattern,  getPatternOptions( ignoreCase ) ),
    foreColor_( foreColor ),
    backColor_( backColor ), enabled_( true )
{
    LOG(logDEBUG) << "New Filter, fore: " << foreColor_.name().toUtf8().constData()
        << " back: " << backColor_.name().toUtf8().constData();
}

QString Filter::pattern() const
{
    return regexp_.pattern();
}

void Filter::setPattern( const QString& pattern )
{
    regexp_.setPattern( pattern );
}

bool Filter::ignoreCase() const
{
    return regexp_.patternOptions().testFlag(QRegularExpression::CaseInsensitiveOption);
}

void Filter::setIgnoreCase( bool ignoreCase )
{
    regexp_.setPatternOptions( getPatternOptions( ignoreCase ) );
}

const QColor& Filter::foreColor() const
{
    return foreColor_;
}

void Filter::setForeColor( const QColor& foreColor )
{
    foreColor_ = foreColor;
}

const QColor& Filter::backColor() const
{
    return backColor_;
}

void Filter::setBackColor( const QColor& backColor )
{
    backColor_ = backColor;
}

bool Filter::hasMatch( const QString& string ) const
{
    return regexp_.match( string ).hasMatch();
}

//
// Operators for serialization
//

QDataStream& operator<<( QDataStream& out, const Filter& object )
{
    LOG(logDEBUG) << "<<operator from Filter";
    out << object.regexp_;
    out << object.foreColor_;
    out << object.backColor_;

    return out;
}

QDataStream& operator>>( QDataStream& in, Filter& object )
{
    LOG(logDEBUG) << ">>operator from Filter";
    in >> object.regexp_;
    in >> object.foreColor_;
    in >> object.backColor_;
    return in;
}


// Default constructor
FilterSet::FilterSet()
{
    qRegisterMetaTypeStreamOperators<Filter>( "Filter" );
    qRegisterMetaTypeStreamOperators<FilterSet>( "FilterSet" );
    qRegisterMetaTypeStreamOperators<FilterSet::FilterList>( "FilterSet::FilterList" );
}

bool FilterSet::matchLine( const QString& line,
        QColor* foreColor, QColor* backColor ) const
{
    for ( QList<Filter>::const_iterator i = filterList.constBegin();
          i != filterList.constEnd(); i++ ) {
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

QDataStream& operator<<( QDataStream& out, const FilterSet& object )
{
    LOG(logDEBUG) << "<<operator from FilterSet";
    out << object.filterList;

    return out;
}

QDataStream& operator>>( QDataStream& in, FilterSet& object )
{
    LOG(logDEBUG) << ">>operator from FilterSet";
    in >> object.filterList;

    return in;
}

//
// Persistable virtual functions implementation
//

void Filter::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "Filter::saveToStorage";

    settings.setValue( "regexp", regexp_.pattern() );
    settings.setValue( "ignore_case", regexp_.patternOptions().testFlag( QRegularExpression::CaseInsensitiveOption ) );

    // save colors as user friendly strings in config
    settings.setValue( "fore_colour", foreColor_.name() );
    settings.setValue( "back_colour", backColor_.name() );
}

void Filter::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "Filter::retrieveFromStorage";

    regexp_ = QRegularExpression( settings.value( "regexp" ).toString(),
                       getPatternOptions( settings.value( "ignore_case", false ).toBool() ) );

    // this code is backwards compatible with FILTERSET_VERSION 1 where colors 
    // are stored as named SVG color strings like "Black" in addition 
    // to version 2 where colors are stored in hex string like #000000
    foreColor_ = QColor( settings.value( "fore_colour" ).toString() );
    backColor_ = QColor( settings.value( "back_colour" ).toString() );
}

void FilterSet::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "FilterSet::saveToStorage";

    settings.beginGroup( "FilterSet" );
    // Remove everything in case the array is shorter than the previous one
    settings.remove("");
    settings.setValue( "version", FILTERSET_VERSION );
    settings.beginWriteArray( "filters" );
    for (int i = 0; i < filterList.size(); ++i) {
        settings.setArrayIndex(i);
        filterList[i].saveToStorage( settings );
    }
    settings.endArray();
    settings.endGroup();
}

void FilterSet::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "FilterSet::retrieveFromStorage";

    filterList.clear();

    if ( settings.contains( "FilterSet/version" ) ) {
        settings.beginGroup( "FilterSet" );

        // check filterset version, the current policy is to always be backwards compatible
        if ( settings.value( "version" ) <= FILTERSET_VERSION ) {
            int size = settings.beginReadArray( "filters" );
            for (int i = 0; i < size; ++i) {
                settings.setArrayIndex(i);
                Filter filter;
                filter.retrieveFromStorage( settings );
                filterList.append( filter );
            }
            settings.endArray();
        }
        else {
            LOG(logERROR) << "Unknown version of FilterSet, ignoring it...";
        }
        settings.endGroup();
    }
    else {
        LOG(logWARNING) << "Trying to import legacy (<=0.8.2) filters...";
        FilterSet tmp_filter_set =
            settings.value( "filterSet" ).value<FilterSet>();
        *this = tmp_filter_set;
        LOG(logWARNING) << "...imported filterset: "
            << filterList.count() << " elements";
        // Remove the old key once migration is done
        settings.remove( "filterSet" );
        // And replace it with the new one
        saveToStorage( settings );
        settings.sync();
    }
}

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

#include "log.h"
#include "filterset.h"

const int FilterSet::FILTERSET_VERSION = 1;

Filter::Filter()
{
}

Filter::Filter( const QString& pattern,
            const QString& foreColorName, const QString& backColorName ) :
    regexp_( pattern ), foreColorName_( foreColorName ),
    backColorName_( backColorName ), enabled_( true )
{
    LOG(logDEBUG) << "New Filter, fore: " << foreColorName_.toStdString()
        << " back: " << backColorName_.toStdString();
}

QString Filter::pattern() const
{
    return regexp_.pattern();
}

void Filter::setPattern( const QString& pattern )
{
    regexp_.setPattern( pattern );
}

const QString& Filter::foreColorName() const
{
    return foreColorName_;
}

void Filter::setForeColor( const QString& foreColorName )
{
    foreColorName_ = foreColorName;
}

const QString& Filter::backColorName() const
{
    return backColorName_;
}

void Filter::setBackColor( const QString& backColorName )
{
    backColorName_ = backColorName;
}

int Filter::indexIn( const QString& string ) const
{
    return regexp_.indexIn( string );
}

//
// Operators for serialization
//

QDataStream& operator<<( QDataStream& out, const Filter& object )
{
    LOG(logDEBUG) << "<<operator from Filter";
    out << object.regexp_;
    out << object.foreColorName_;
    out << object.backColorName_;

    return out;
}

QDataStream& operator>>( QDataStream& in, Filter& object )
{
    LOG(logDEBUG) << ">>operator from Filter";
    in >> object.regexp_;
    in >> object.foreColorName_;
    in >> object.backColorName_;

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
        if ( i->indexIn( line ) != -1 ) {
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
    settings.setValue( "fore_colour", foreColorName_ );
    settings.setValue( "back_colour", backColorName_ );
}

void Filter::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "Filter::retrieveFromStorage";

    regexp_ = QRegExp( settings.value( "regexp" ).toString() );
    foreColorName_ = settings.value( "fore_colour" ).toString();
    backColorName_ = settings.value( "back_colour" ).toString();
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
        if ( settings.value( "version" ) == FILTERSET_VERSION ) {
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

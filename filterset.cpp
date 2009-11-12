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

// This file implements classes Filter and FilterSet

#include "log.h"

#include "filterset.h"

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
    out << object.filterList;

    return out;
}

QDataStream& operator>>( QDataStream& in, FilterSet& object )
{
    in >> object.filterList;

    return in;
}

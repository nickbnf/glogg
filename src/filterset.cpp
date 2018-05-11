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
#include <QSize>
#include <QPixmap>
#include <QIcon>

#include "log.h"
#include "filterset.h"

// Filterset Version History:
// --------------------------
// - Version 2 Changes:
//     - Added filter description.
//     - Made the 'enabled' flag persistent.

const int FilterSet::FILTERSET_VERSION_2 = 2;
const int FilterSet::FILTERSET_VERSION_1 = 1;

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
            const QString& foreColorName, const QString& backColorName , const QString &descriptionStr) :
    regexp_( pattern,  getPatternOptions( ignoreCase ) ),
    foreColorName_( foreColorName ),
    backColorName_( backColorName ),
    description_( descriptionStr ),
    enabled_( true )
{
    LOG(logDEBUG) << "New Filter, fore: " << foreColorName_.toStdString()
        << " back: " << backColorName_.toStdString()
        << " description: " << description_.toStdString();
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

const QString &Filter::description() const
{
    return description_;
}

void Filter::setDescription(const QString &descriptionStr)
{
    description_ = descriptionStr;
}

bool Filter::enabled() const
{
    return enabled_;
}

void Filter::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

bool Filter::hasMatch( const QString& string ) const
{
    if ( enabled_ )
    {
        return regexp_.match( string ).hasMatch();
    }
    return false;
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

FilterSet::FilterSet(const FilterSet &f)
    : QAbstractItemModel(nullptr),
      filterList_( f.filterList_ )
{
    qRegisterMetaTypeStreamOperators<Filter>( "Filter" );
    qRegisterMetaTypeStreamOperators<FilterSet>( "FilterSet" );
    qRegisterMetaTypeStreamOperators<FilterSet::FilterList>( "FilterSet::FilterList" );
}

FilterSet &FilterSet::operator=(const FilterSet &f)
{
    if ( &f != this )
    {
        filterList_ = f.filterList_;
    }
    return *this;
}

bool FilterSet::matchLine( const QString& line,
        QColor* foreColor, QColor* backColor ) const
{
    for ( QList<Filter>::const_iterator i = filterList_.constBegin();
          i != filterList_.constEnd(); i++ ) {
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

QDataStream& operator<<( QDataStream& out, const FilterSet& object )
{
    LOG(logDEBUG) << "<<operator from FilterSet";
    out << object.filterList_;

    return out;
}

QDataStream& operator>>( QDataStream& in, FilterSet& object )
{
    LOG(logDEBUG) << ">>operator from FilterSet";
    in >> object.filterList_;

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
    settings.setValue( "fore_colour", foreColorName_ );
    settings.setValue( "back_colour", backColorName_ );
    settings.setValue( "description", description_ );
    settings.setValue( "enabled", enabled_ );
}

void Filter::retrieveFromStorage( QSettings& settings, int version )
{
    LOG(logDEBUG) << "Filter::retrieveFromStorage";

    regexp_ = QRegularExpression( settings.value( "regexp" ).toString(),
                       getPatternOptions( settings.value( "ignore_case", false ).toBool() ) );
    foreColorName_ = settings.value( "fore_colour" ).toString();
    backColorName_ = settings.value( "back_colour" ).toString();

    if ( version >= FilterSet::FILTERSET_VERSION_2 )
    {
        description_ = settings.value( "description" ).toString();
        enabled_ = settings.value( "enabled" ).toBool();
    }
}

void FilterSet::saveToStorage( QSettings& settings ) const
{
    LOG(logDEBUG) << "FilterSet::saveToStorage";

    settings.beginGroup( "FilterSet" );
    // Remove everything in case the array is shorter than the previous one
    settings.remove("");
    settings.setValue( "version", FILTERSET_VERSION_2 );
    settings.beginWriteArray( "filters" );
    for (int i = 0; i < filterList_.size(); ++i) {
        settings.setArrayIndex(i);
        filterList_[i].saveToStorage( settings );
    }
    settings.endArray();
    settings.endGroup();
}

void FilterSet::retrieveFromStorage( QSettings& settings )
{
    LOG(logDEBUG) << "FilterSet::retrieveFromStorage";

    filterList_.clear();

    if ( settings.contains( "FilterSet/version" ) ) {
        settings.beginGroup( "FilterSet" );
        int version = settings.value( "version" ).toInt();
        if ( ( version == FILTERSET_VERSION_1 ) || ( version == FILTERSET_VERSION_2 ) ) {
            int size = settings.beginReadArray( "filters" );
            for (int i = 0; i < size; ++i) {
                settings.setArrayIndex(i);
                Filter filter;
                filter.retrieveFromStorage( settings, version );
                filterList_.append( filter );
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
            << filterList_.count() << " elements";
        // Remove the old key once migration is done
        settings.remove( "filterSet" );
        // And replace it with the new one
        saveToStorage( settings );
        settings.sync();
    }
}

void FilterSet::addFilter(const Filter &filter)
{
    filterList_.append( Filter(filter) );

    int row = filterList_.size()-1;
    QModelIndex topLeft = index(row, 0, QModelIndex());
    QModelIndex bottomRight = index(row, 0, QModelIndex());
    emit dataChanged(topLeft, bottomRight);
    emit layoutChanged();
}

void FilterSet::removeFilter(int filterIndex)
{
    if ( filterIndex >= 0 && filterIndex < filterList_.size() )
    {
        filterList_.removeAt( filterIndex );

        QModelIndex topLeft = index(filterIndex, 0, QModelIndex());
        QModelIndex bottomRight = index(filterList_.size()-1, 0, QModelIndex());
        emit dataChanged(topLeft, bottomRight);
        emit layoutChanged();
    }
}

void FilterSet::moveFilterUp(int filterIndex)
{
    if ( filterIndex > 0 ) {
        filterList_.move( filterIndex, filterIndex - 1 );

        QModelIndex topLeft = index(filterIndex-1, 0, QModelIndex());
        QModelIndex bottomRight = index(filterIndex, 0, QModelIndex());
        emit dataChanged(topLeft, bottomRight);
        emit layoutChanged();
    }
}

void FilterSet::moveFilterDown(int filterIndex)
{
    if ( ( filterIndex >= 0 ) && ( filterIndex < filterList_.size()-1 ) ) {
        filterList_.move( filterIndex, filterIndex + 1 );

        QModelIndex topLeft = index(filterIndex, 0, QModelIndex());
        QModelIndex bottomRight = index(filterIndex+1, 0, QModelIndex());
        emit dataChanged(topLeft, bottomRight);
        emit layoutChanged();
    }
}

QModelIndex FilterSet::index(int row, int column, const QModelIndex &parent) const
{
    if ( row >= 0 && row < filterList_.size() )
    {
        const Filter& f = filterList_.at(row);
        return createIndex( row, column, const_cast<void*>(static_cast<const void*>(&f)) );
    }
    return QModelIndex();
}

QModelIndex FilterSet::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int FilterSet::rowCount(const QModelIndex &parent) const
{
    return filterList_.size();
}

int FilterSet::columnCount(const QModelIndex &parent) const
{
    return 6;
}

QVariant FilterSet::data(const QModelIndex &index, int role) const
{
    QVariant value;
    if ( index.isValid() )
    {
        if ( index.row() >= 0 && index.row() < filterList_.size() )
        {
            const Filter& f = filterList_[index.row()];
            if ( role == Qt::DisplayRole || role == Qt::EditRole )
            {
                switch (index.column())
                {
                case FILTER_COLUMN_DESCRIPTION:
                    value = QVariant( f.description() );
                    break;
                case FILTER_COLUMN_PATTERN:
                    value = QVariant( f.pattern() );
                    break;
                case FILTER_COLUMN_FOREGROUND:
                    value = QVariant( f.foreColorName() );
                    break;
                case FILTER_COLUMN_BACKGROUND:
                    value = QVariant( f.backColorName() );
                    break;
                default:
                    break;
                }
            }
            else if ( role == Qt::CheckStateRole )
            {
                switch (index.column())
                {
                case FILTER_COLUMN_ENABLED:
                    value = f.enabled() ? Qt::Checked : Qt::Unchecked;
                    break;
                case FILTER_COLUMN_IGNORE_CASE:
                    value = f.ignoreCase() ? Qt::Checked : Qt::Unchecked;
                    break;
                default:
                    break;
                }
            }
            else if ( role == Qt::DecorationRole )
            {
                if ( index.column() == 4 || index.column() == 5 )
                {
                    QPixmap solidPixmap( 20, 10 );
                    switch (index.column())
                    {
                    case FILTER_COLUMN_FOREGROUND:
                        solidPixmap.fill( QColor( f.foreColorName() ));
                        break;
                    case FILTER_COLUMN_BACKGROUND:
                        solidPixmap.fill( QColor( f.backColorName() ));
                        break;
                    default:
                        break;
                    }
                    QIcon solidIcon { solidPixmap };
                    value = solidIcon;
                }
            }
            else if ( role == Qt::TextColorRole )
            {
                // Colorize only the description.
                if ( index.column() == FILTER_COLUMN_DESCRIPTION )
                {
                    value = QColor( f.foreColorName() );
                }
            }
            else if ( role == Qt::BackgroundColorRole )
            {
                // Colorize only the description.
                if ( index.column() == FILTER_COLUMN_DESCRIPTION )
                {
                    value = QColor( f.backColorName() );
                }
            }
        }
    }
    return value;
}

bool FilterSet::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool changed = false;
    if ( index.isValid() )
    {
        if ( index.row() >= 0 && index.row() < filterList_.size() )
        {
            Filter& f = filterList_[index.row()];
            if ( role == Qt::CheckStateRole )
            {
                if ( index.column() == FILTER_COLUMN_ENABLED )
                {
                    f.setEnabled( value.toBool() );
                    changed = true;
                }
                else if ( index.column() == FILTER_COLUMN_IGNORE_CASE )
                {
                    f.setIgnoreCase( value.toBool() );
                    changed = true;
                }
            }
            else if ( role == Qt::EditRole )
            {
                switch (index.column())
                {
                case FILTER_COLUMN_DESCRIPTION:
                    f.setDescription( value.toString() );
                    changed = true;
                    break;
                case FILTER_COLUMN_PATTERN:
                    f.setPattern( value.toString() );
                    changed = true;
                    break;
                case FILTER_COLUMN_FOREGROUND:
                    f.setForeColor( value.toString() );
                    changed = true;
                    break;
                case FILTER_COLUMN_BACKGROUND:
                    f.setBackColor( value.toString() );
                    changed = true;
                    break;
                }
            }
        }
    }
    if ( changed )
    {
        emit dataChanged(index, index);
    }

    return changed;
}

QVariant FilterSet::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant value;
    if ( orientation == Qt::Horizontal)
    {
        if ( role == Qt::DisplayRole )
        {
            switch (section)
            {
            case FILTER_COLUMN_ENABLED:
                value = tr("Enabled");
                break;
            case FILTER_COLUMN_DESCRIPTION:
                value = tr("Description");
                break;
            case FILTER_COLUMN_PATTERN:
                value = tr("Pattern");
                break;
            case FILTER_COLUMN_IGNORE_CASE:
                value = tr("Ignore Case");
                break;
            case FILTER_COLUMN_FOREGROUND:
                value = tr("Foreground");
                break;
            case FILTER_COLUMN_BACKGROUND:
                value = tr("Background");
                break;
            default:
                break;
            }
        }
        else if ( role == Qt::SizeHintRole )
        {
            switch (section)
            {
            case FILTER_COLUMN_ENABLED:
                value = QSize(40, 20);
                break;
            case FILTER_COLUMN_DESCRIPTION:
                value = QSize(300, 20);
                break;
            case FILTER_COLUMN_PATTERN:
                value = QSize(300, 20);
                break;
            case FILTER_COLUMN_IGNORE_CASE:
                value = QSize(40, 20);
                break;
            case FILTER_COLUMN_FOREGROUND:
                value = QSize(80, 20);
                break;
            case FILTER_COLUMN_BACKGROUND:
                value = QSize(80, 20);
                break;
            default:
                break;
            }
        }
    }
    return value;
}

Qt::ItemFlags FilterSet::flags(const QModelIndex &index) const
{
    Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index);
    if ( index.isValid() )
    {
        if ( index.row() >= 0 && index.row() < filterList_.size() )
        {
            switch (index.column())
            {
            case FILTER_COLUMN_ENABLED:
                itemFlags = itemFlags | Qt::ItemIsUserCheckable;
                break;
            case FILTER_COLUMN_DESCRIPTION:
                itemFlags = itemFlags | Qt::ItemIsEditable;
                break;
            case FILTER_COLUMN_PATTERN:
                itemFlags = itemFlags | Qt::ItemIsEditable;
                break;
            case FILTER_COLUMN_IGNORE_CASE:
                itemFlags = itemFlags | Qt::ItemIsUserCheckable;
                break;
            case FILTER_COLUMN_FOREGROUND:
                itemFlags = itemFlags | Qt::ItemIsEditable;
                break;
            case FILTER_COLUMN_BACKGROUND:
                itemFlags = itemFlags | Qt::ItemIsEditable;
                break;
            }
        }
    }
    return itemFlags;
}

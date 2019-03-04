/*
 * Copyright (C) 2009, 2010, 2012, 2017 Nicolas Bonnefon and other contributors
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

// This file implements the FilteredView concrete class.
// Most of the actual drawing and event management is done in AbstractLogView
// Only behaviour specific to the filtered (bottom) view is implemented here.

#include <cassert>

#include "filteredview.h"

FilteredView::FilteredView( LogFilteredData* newLogData,
        const QuickFindPattern* const quickFindPattern, QWidget* parent )
    : AbstractLogView( newLogData, quickFindPattern, parent )
{
    // We keep a copy of the filtered data for fast lookup of the line type
    logFilteredData_ = newLogData;
}

void FilteredView::setVisibility( Visibility visi )
{
    assert( logFilteredData_ );

    LogFilteredData::Visibility data_visibility =
        LogFilteredData::MarksAndMatches;
    switch ( visi ) {
        case MarksOnly:
            data_visibility = LogFilteredData::MarksOnly;
            break;
        case MatchesOnly:
            data_visibility = LogFilteredData::MatchesOnly;
            break;
        case MarksAndMatches:
            data_visibility = LogFilteredData::MarksAndMatches;
            break;
    };

    logFilteredData_->setVisibility( data_visibility );

    updateData();
}

// For the filtered view, a line is always matching!
AbstractLogView::LineType FilteredView::lineType( int lineNumber ) const
{
    LogFilteredData::FilteredLineType type =
        logFilteredData_->filteredLineTypeByIndex( lineNumber );
    if ( type & LogFilteredData::Mark ) // If both Mark and Match, Mark wins
        return Marked;
    else
        return Match;
}

qint64 FilteredView::displayLineNumber( int lineNumber ) const
{
    // Display a 1-based index
    return logFilteredData_->getMatchingLineNumber( lineNumber ) + 1;
}

qint64 FilteredView::maxDisplayLineNumber() const
{
    return logFilteredData_->getNbTotalLines();
}

void FilteredView::keyPressEvent( QKeyEvent* keyEvent )
{
    bool noModifier = keyEvent->modifiers() == Qt::NoModifier;

    if ( keyEvent->key() == Qt::Key_BracketLeft && noModifier ) {
        for ( qint64 i = static_cast<qint64>( getViewPosition() ) - 1; i >= 0; --i ) {
            if ( lineType( i ) == Marked ) {
                selectAndDisplayLine( static_cast<LineNumber>( i ) );
                break;
            }
        }
        keyEvent->accept();
    }
    else if ( keyEvent->key() == Qt::Key_BracketRight && noModifier ) {
        for ( qint64 i = getViewPosition() + 1;
                i < logFilteredData_->getNbLine(); ++i ) {
            if ( lineType( i ) == Marked ) {
                selectAndDisplayLine( static_cast<LineNumber>( i ) );
                break;
            }
        }
        keyEvent->accept();
    }
    else {
        keyEvent->ignore();
        AbstractLogView::keyPressEvent( keyEvent );
    }
}

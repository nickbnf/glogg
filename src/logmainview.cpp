/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2017 Nicolas Bonnefon
 * and other contributors
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

// This file implements the LogMainView concrete class.
// Most of the actual drawing and event management is done in AbstractLogView
// Only behaviour specific to the main (top) view is implemented here.

#include "logmainview.h"

#include "data/logfiltereddata.h"
#include "overview.h"

#include <QKeyEvent>

LogMainView::LogMainView(PythonPluginInterface* pp, const LogData* newLogData,
        const QuickFindPattern* const quickFindPattern,
        Overview* overview,
        OverviewWidget* overview_widget,
        QWidget* parent)
    : AbstractLogView(pp, newLogData, quickFindPattern, parent )
{
    filteredData_ = NULL;

    // The main data has a real (non NULL) Overview
    setOverview( overview, overview_widget );
}

// Just update our internal record.
void LogMainView::useNewFiltering( LogFilteredData* filteredData )
{
    filteredData_ = filteredData;

    if ( getOverview() != NULL )
        getOverview()->setFilteredData( filteredData_ );
}

AbstractLogView::LineType LogMainView::lineType( int lineNumber ) const
{
    if ( filteredData_ != NULL ) {
        LineType line_type;
        if ( filteredData_->isLineMarked( lineNumber ) )
            line_type = Marked;
        else if ( filteredData_->isLineInMatchingList( lineNumber ) )
            line_type = Match;
        else
            line_type = Normal;

        return line_type;
    }
    else
        return Normal;
}

void LogMainView::keyPressEvent( QKeyEvent* keyEvent )
{
    bool noModifier = keyEvent->modifiers() == Qt::NoModifier;

    if ( keyEvent->key() == Qt::Key_BracketLeft && noModifier ) {
        qint64 line = filteredData_->getMarkBefore( getViewPosition() );
        if ( line >= 0 ) {
            selectAndDisplayLine( static_cast<LineNumber>( line ) );
        }
        keyEvent->accept();
    }
    else if ( keyEvent->key() == Qt::Key_BracketRight && noModifier ) {
        qint64 line = filteredData_->getMarkAfter( getViewPosition() );
        if ( line >= 0 ) {
            selectAndDisplayLine( static_cast<LineNumber>( line ) );
        }
        keyEvent->accept();
    }
    else {
        keyEvent->ignore();
        AbstractLogView::keyPressEvent( keyEvent );
    }
}

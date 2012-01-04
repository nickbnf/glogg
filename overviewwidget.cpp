/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

// This file implements OverviewWidget.  This class is responsable for
// managing and painting the matches overview widget.

#include <QPainter>
#include <QMouseEvent>
#include <cassert>

#include "log.h"

#include "overviewwidget.h"

#include "overview.h"

// Graphic parameters
const int OverviewWidget::LINE_MARGIN = 4;
const float OverviewWidget::LINE_OPACITY = 0.35;

OverviewWidget::OverviewWidget( QWidget* parent ) : QWidget( parent )
{
    overview_ = NULL;

    setBackgroundRole( QPalette::Window );

    // We should be hidden by default (e.g. for the FilteredView)
    hide();
}

void OverviewWidget::paintEvent( QPaintEvent* paintEvent )
{
    LOG(logDEBUG) << "OverviewWidget::paintEvent";

    static const QColor match_color("red");
    static const QColor mark_color("dodgerblue");

    // We must be hidden until we have an Overview
    assert( overview_ != NULL );

    overview_->updateView( height() );

    {
        QPainter painter( this );

        // The line separating from the main view
        painter.setPen( palette().color(QPalette::Text) );
        painter.drawLine( 0, 0, 0, height() );

        // Allow multiple matches to look 'darker' than a single one.
        painter.setOpacity( LINE_OPACITY );
        // The 'match' lines
        painter.setPen( match_color );
        foreach (int line, *(overview_->getMatchLines()) ) {
            painter.drawLine( 1 + LINE_MARGIN,
                    line, width() - LINE_MARGIN - 1, line );
        }

        // The 'mark' lines
        painter.setPen( mark_color );
        foreach (int line, *(overview_->getMarkLines()) ) {
            painter.drawLine( 1 + LINE_MARGIN,
                    line, width() - LINE_MARGIN - 1, line );
        }

        // The 'view' lines
        painter.setOpacity( 1 );
        painter.setPen( palette().color(QPalette::Text) );
        std::pair<int,int> view_lines = overview_->getViewLines();
        painter.drawLine( 1, view_lines.first, width(), view_lines.first );
        painter.drawLine( 1, view_lines.second, width(), view_lines.second );
    }
}

void OverviewWidget::mousePressEvent( QMouseEvent* mouseEvent )
{
    if ( mouseEvent->button() == Qt::LeftButton )
        handleMousePress( mouseEvent->y() );
}

void OverviewWidget::mouseMoveEvent( QMouseEvent* mouseEvent )
{
    if ( mouseEvent->buttons() |= Qt::LeftButton )
        handleMousePress( mouseEvent->y() );
}

void OverviewWidget::handleMousePress( int position )
{
    int line = overview_->fileLineFromY( position );
    LOG(logDEBUG) << "OverviewWidget::handleMousePress y=" << position << " line=" << line;
    emit lineClicked( line );
}

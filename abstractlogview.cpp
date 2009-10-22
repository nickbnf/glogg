/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

// This file implements the AbstractLogView base class.
// Most of the actual drawing and event management common to the two views
// is implemented in this class.  The class only calls protected virtual
// functions when view specific behaviour is desired, using the template
// pattern.

#include <iostream>

#include <QFile>
#include <QRect>
#include <QPaintEvent>
#include <QPainter>
#include <QFontMetrics>
#include <QScrollBar>

#include "log.h"

#include "common.h"
#include "configuration.h"
#include "logmainview.h"

AbstractLogView::AbstractLogView(const AbstractLogData* newLogData,
       QWidget* parent) : QAbstractScrollArea(parent)
{
    logData = newLogData;

    // Create the viewport QWidget
    setViewport(0);
    setAttribute(Qt::WA_StaticContents);  // Does it work?

    firstLine = 0;
    lastLine = 0;
    firstCol = 0;
    selectedLine = -1;
}

//
// Received events
//

void AbstractLogView::mousePressEvent( QMouseEvent* mouseEvent )
{
    // Selection implementation
    if ( mouseEvent->button() == Qt::LeftButton )
    {
        int line = convertCoordToLine( mouseEvent->y() );
        if ( line < logData->getNbLine() ) {
            selectedLine = line;
            emit newSelection( line );
        }
    }
}

void AbstractLogView::resizeEvent(QResizeEvent* resizeEvent)
{
    if ( logData == NULL )
        return;

    LOG(logDEBUG) << "resizeEvent received";

    updateDisplaySize();
}

void AbstractLogView::scrollContentsBy( int dx, int dy )
{
    LOG(logDEBUG) << "scrollContentsBy received";

    firstLine -= dy;
    firstCol  -= dx;
    lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );

    update();
}

void AbstractLogView::paintEvent(QPaintEvent* paintEvent)
{
    QRect invalidRect = paintEvent->rect();
    if ( (invalidRect.isEmpty()) || (logData == NULL) )
        return;

    LOG(logDEBUG) << "paintEvent received, firstLine=" << firstLine
        << " lastLine=" << lastLine <<
        " rect: " << invalidRect.topLeft().x() <<
        ", " << invalidRect.topLeft().y() <<
        ", " <<invalidRect.bottomRight().x() <<
        ", " << invalidRect.bottomRight().y();

    {
        // Repaint the viewport
        QPainter painter( viewport() );
        const int fontHeight = painter.fontMetrics().height();
        const int fontAscent = painter.fontMetrics().ascent();
        const int nbCols = getNbVisibleCols();
        const QPalette& palette = viewport()->palette();
        const FilterSet& filterSet = Config().filterSet();
        QColor foreColor, backColor;

        const QBrush normalBulletBrush = QBrush( Qt::white );
        const QBrush matchBulletBrush = QBrush( Qt::red );

        // Params
        const int bulletLineX = 11;  // Looks better with an odd value

        // First draw the bullet left margin
        painter.setPen(palette.color(QPalette::Text));
        painter.drawLine( bulletLineX, 0, bulletLineX, viewport()->height() );
        painter.fillRect( 0, 0, bulletLineX, viewport()->height(), Qt::darkGray );

        // Then draw each line
        for (int i = firstLine; i < lastLine; i++) {
            // Position in pixel of the base line of the line to print
            const int yPos = (i-firstLine) * fontHeight;
            const int xPos = bulletLineX + 2;

            // string to print, cut to fit the length and position of the view
            const QString cutLine = logData->getLineString(i).mid(firstCol, firstCol+nbCols);

            if ( i == selectedLine ) {
                // Reverse the selected line
                foreColor = palette.color( QPalette::HighlightedText );
                backColor = palette.color( QPalette::Highlight );
                painter.setPen(palette.color(QPalette::Text));
            }
            else if ( filterSet.matchLine( logData->getLineString(i),
                        &foreColor, &backColor ) ) {
                // Apply a filter to the line
            }
            else {
                // Use the default colors
                foreColor = palette.color( QPalette::Text );
                backColor = palette.color( QPalette::Base );
            }

            painter.fillRect( xPos, yPos, viewport()->width(),
                    fontHeight, backColor );
            painter.setPen( foreColor );
            painter.drawText( xPos, yPos + fontAscent, cutLine );

            // Then draw the bullet
            const int circleSize = 3;
            const QPoint circleCenter =
                QPoint( bulletLineX / 2, yPos + (fontHeight / 2) );

            if ( isLineMatching( i ) )
                painter.setBrush( matchBulletBrush );
            else
                painter.setBrush( normalBulletBrush );
            painter.setPen( palette.color( QPalette::Text ) );
            painter.drawEllipse( circleCenter, circleSize, circleSize );
        }
    }
    LOG(logDEBUG) << "End of repaint";
}

//
// Public functions
//

void AbstractLogView::updateData(const AbstractLogData* newLogData)
{
    // Save the new data
    logData = newLogData;

    // Adapt the scroll bars to the new content
    verticalScrollBar()->setValue(0);
    verticalScrollBar()->setRange(0, logData->getNbLine()-1);
    const int hScrollMaxValue = ( logData->getMaxLength() - getNbVisibleCols() + 1 ) > 0 ?
        ( logData->getMaxLength() - getNbVisibleCols() + 1 ) : 0;
    horizontalScrollBar()->setValue(0);
    horizontalScrollBar()->setRange(0, hScrollMaxValue);

    // Unselect any line and show the upper left corner
    firstLine = 0;
    lastLine = min2(logData->getNbLine(), firstLine + getNbVisibleLines());
    firstCol = 0;
    selectedLine = -1;

    // Repaint!
    update();
}

void AbstractLogView::updateDisplaySize()
{
    // Calculate the index of the last line shown
    lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );

    // Update the scroll bars
    verticalScrollBar()->setPageStep( getNbVisibleLines() );

    const int hScrollMaxValue = ( logData->getMaxLength() - getNbVisibleCols() + 1 ) > 0 ?
        ( logData->getMaxLength() - getNbVisibleCols() + 1 ) : 0;
    LOG(logDEBUG) << "getMaxLength=" << logData->getMaxLength();
    LOG(logDEBUG) << "getNbVisibleCols=" << getNbVisibleCols();
    LOG(logDEBUG) << "hScrollMaxValue=" << hScrollMaxValue;
    horizontalScrollBar()->setRange( 0,  hScrollMaxValue );
}

// Select the line and ensure it is visible on the screen, scrolling if not.
void AbstractLogView::displayLine( int line )
{
    LOG(logDEBUG) << "displayLine " << line << " nbLines: " << logData->getNbLine();

    selectedLine = line;

    // If the line is already the screen
    if ( ( line >= firstLine ) &&
         ( line < ( firstLine + getNbVisibleLines() ) ) ) {
        // ... don't scroll and just repaint
        update();
    } else {
        // Put the selected line in the middle if possible
        int newTopLine = line - ( getNbVisibleLines() / 2 );
        if ( newTopLine < 0 )
            newTopLine = 0;

        // This will also trigger a scrollContents event
        verticalScrollBar()->setValue( newTopLine );
    }
}

//
// Private functions
//

// Returns the number of lines visible in the viewport
int AbstractLogView::getNbVisibleLines() const
{
    QFontMetrics fm = fontMetrics();
    return viewport()->height()/fm.height() + 1;
}

// Returns the number of columns visible in the viewport
int AbstractLogView::getNbVisibleCols() const
{
    QFontMetrics fm = fontMetrics();
    return viewport()->width()/fm.width('w') + 1;
}

// Converts the mouse x, y coordinates to the line number in the file
int AbstractLogView::convertCoordToLine(int yPos) const
{
    QFontMetrics fm = fontMetrics();
    int line = firstLine + yPos/fm.height();

    return line;
}

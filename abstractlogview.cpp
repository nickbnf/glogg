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

inline void LineDrawer::addChunk( int first_col, int last_col,
        QColor fore, QColor back )
{
    if ( first_col < 0 )
        first_col = 0;
    int length = last_col - first_col;
    if ( length > 0 ) {
        list << Chunk ( first_col, length, fore, back );
    }
}

inline void LineDrawer::draw( QPainter& painter,
        int xPos, int yPos, int line_width, const QString& line )
{
    QFontMetrics fm = painter.fontMetrics();
    const int fontHeight = fm.height();
    const int fontAscent = fm.ascent();

    foreach ( Chunk chunk, list ) {
        // Draw each chunk
        LOG(logDEBUG) << "Chunk: " << chunk.start() << " " << chunk.length();
        QString cutline = line.mid( chunk.start(), chunk.length() );
        const int chunk_width = fm.width( cutline );
        painter.fillRect( xPos, yPos, chunk_width,
                fontHeight, chunk.backColor() );
        painter.setPen( chunk.foreColor() );
        painter.drawText( xPos, yPos + fontAscent, cutline );
        xPos += chunk_width;
    }

    // Draw the empty block at the end of the line
    int blank_width = line_width - xPos;
    LOG(logDEBUG) << "blank_width: " << blank_width;

    if ( blank_width > 0 )
        painter.fillRect( xPos, yPos, blank_width, fontHeight, backColor_ );
}

// Looks better with an odd value
const int AbstractLogView::bulletLineX_ = 11;
const int AbstractLogView::leftMarginPx_ = bulletLineX_ + 2;

AbstractLogView::AbstractLogView(const AbstractLogData* newLogData,
        QWidget* parent) : QAbstractScrollArea(parent),
        selectionStartPos_(), selectionCurrentEndPos_(),
        autoScrollTimer_(), selection_()
{
    logData = newLogData;

    // Create the viewport QWidget
    setViewport(0);
    setAttribute(Qt::WA_StaticContents);  // Does it work?

    selectionStarted_ = false;

    firstLine = 0;
    lastLine = 0;
    firstCol = 0;
}

//
// Received events
//

void AbstractLogView::changeEvent( QEvent* changeEvent )
{
    QAbstractScrollArea::changeEvent( changeEvent );

    // Stop the timer if the widget becomes inactive
    if ( changeEvent->type() == QEvent::ActivationChange ) {
        if ( ! isActiveWindow() )
            autoScrollTimer_.stop();
    }
    viewport()->update();
}

void AbstractLogView::mousePressEvent( QMouseEvent* mouseEvent )
{
    // Selection implementation
    if ( mouseEvent->button() == Qt::LeftButton )
    {
        int line = convertCoordToLine( mouseEvent->y() );
        if ( line < logData->getNbLine() ) {
            selection_.selectLine( line );
            emit newSelection( line );
        }

        // Remember the click in case we're starting a selection
        selectionStarted_ = true;
        selectionStartPos_ = convertCoordToFilePos( mouseEvent->pos() );
    }
}

void AbstractLogView::mouseMoveEvent( QMouseEvent* mouseEvent )
{
    // Selection implementation
    if ( selectionStarted_ )
    {
        int deltaX = 0;
        int deltaY = 0;

        int fontWidth = fontMetrics().width('i');

        QPoint thisEndPos = convertCoordToFilePos( mouseEvent->pos() );
        if ( thisEndPos != selectionCurrentEndPos_ )
        {
            // Are we on a different line?
            if ( selectionStartPos_.y() != thisEndPos.y() )
            {
                if ( thisEndPos.y() != selectionCurrentEndPos_.y() )
                {
                    // This is a 'range' selection
                    LOG(logDEBUG) << "range selection: "
                        << selectionStartPos_.y() << " " << thisEndPos.y();
                    selection_.selectRange( selectionStartPos_.y(),
                            thisEndPos.y() );
                    update();
                }
            }
            // Are we on a different column?
            else if ( selectionStartPos_.x() != thisEndPos.x() )
            {
                if ( thisEndPos.x() != selectionCurrentEndPos_.x() )
                {
                    // This is a 'portion' selection
                    LOG(logDEBUG) << "portion selection: "
                        << selectionStartPos_.x() << " " << thisEndPos.x();
                    selection_.selectPortion( thisEndPos.y(),
                            selectionStartPos_.x(), thisEndPos.x() );
                    update();
                }
            }
            selectionCurrentEndPos_ = thisEndPos;

            // Do we need to scroll while extending the selection?
            QRect visible = viewport()->rect();
            if ( visible.contains( mouseEvent->pos() ) )
                autoScrollTimer_.stop();
            else if ( ! autoScrollTimer_.isActive() )
                autoScrollTimer_.start( 100, this );
        }
    }
}

void AbstractLogView::mouseReleaseEvent( QMouseEvent* )
{
    if ( autoScrollTimer_.isActive() )
        autoScrollTimer_.stop();
}

void AbstractLogView::timerEvent( QTimerEvent* timerEvent )
{
    if ( timerEvent->timerId() == autoScrollTimer_.timerId() ) {
        QRect visible = viewport()->rect();
        const QPoint globalPos = QCursor::pos();
        const QPoint pos = viewport()->mapFromGlobal( globalPos );
        QMouseEvent ev( QEvent::MouseMove, pos, globalPos, Qt::LeftButton,
                Qt::LeftButton, Qt::NoModifier );
        mouseMoveEvent( &ev );
        int deltaX = qMax( pos.x() - visible.left(),
                visible.right() - pos.x() ) - visible.width();
        int deltaY = qMax( pos.y() - visible.top(),
                visible.bottom() - pos.y() ) - visible.height();
        int delta = qMax( deltaX, deltaY );

        if ( delta >= 0 ) {
            if ( delta < 7 )
                delta = 7;
            int timeout = 4900 / ( delta * delta );
            autoScrollTimer_.start( timeout, this );

            if ( deltaX > 0 )
                horizontalScrollBar()->triggerAction(
                        pos.x() <visible.center().x() ?
                        QAbstractSlider::SliderSingleStepSub :
                        QAbstractSlider::SliderSingleStepAdd );

            if ( deltaY > 0 )
                verticalScrollBar()->triggerAction(
                        pos.y() <visible.center().y() ?
                        QAbstractSlider::SliderSingleStepSub :
                        QAbstractSlider::SliderSingleStepAdd );
        }
    }
    QAbstractScrollArea::timerEvent( timerEvent );
}

void AbstractLogView::keyPressEvent( QKeyEvent* keyEvent )
{
    LOG(logDEBUG4) << "keyPressEvent received";

    if ( keyEvent->key() == Qt::Key_Left )
        horizontalScrollBar()->triggerAction(QScrollBar::SliderPageStepSub);
    else if ( keyEvent->key() == Qt::Key_Right )
        horizontalScrollBar()->triggerAction(QScrollBar::SliderPageStepAdd);
    else {
        switch ( (keyEvent->text())[0].toAscii() ) {
            case 'j':
                //verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepAdd);
                moveSelection( 1 );
                break;
            case 'k':
                //verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepSub);
                moveSelection( -1 );
                break;
            case 'h':
                horizontalScrollBar()->triggerAction(QScrollBar::SliderSingleStepSub);
                break;
            case 'l':
                horizontalScrollBar()->triggerAction(QScrollBar::SliderSingleStepAdd);
                break;
            case 'g':
                jumpToTop();
                break;
            case 'G':
                jumpToBottom();
                break;
            default:
                keyEvent->ignore();
        }
    }

    if ( !keyEvent->isAccepted() )
        QAbstractScrollArea::keyPressEvent( keyEvent );
}

void AbstractLogView::resizeEvent( QResizeEvent* resizeEvent )
{
    if ( logData == NULL )
        return;

    LOG(logDEBUG) << "resizeEvent received";

    updateDisplaySize();
}

void AbstractLogView::scrollContentsBy( int dx, int dy )
{
    LOG(logDEBUG4) << "scrollContentsBy received";

    firstLine = (firstLine - dy) > 0 ? firstLine - dy : 0;
    firstCol  = (firstCol - dx) > 0 ? firstCol - dx : 0;
    lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );

    update();
}

void AbstractLogView::paintEvent( QPaintEvent* paintEvent )
{
    QRect invalidRect = paintEvent->rect();
    if ( (invalidRect.isEmpty()) || (logData == NULL) )
        return;

    LOG(logDEBUG4) << "paintEvent received, firstLine=" << firstLine
        << " lastLine=" << lastLine <<
        " rect: " << invalidRect.topLeft().x() <<
        ", " << invalidRect.topLeft().y() <<
        ", " << invalidRect.bottomRight().x() <<
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

        // First check the lines to be drawn are within range (might not be the case if
        // the file has just changed)
        const int nbLines = logData->getNbLine();
        if ( nbLines == 0 ) {
            return;
        }
        else {
            if ( firstLine >= nbLines )
                firstLine = nbLines - 1;
            if ( lastLine >= nbLines )
                lastLine =  nbLines - 1;
        }

        // Lines to write
        const QStringList lines = logData->getLines( firstLine, lastLine - firstLine + 1 );

        // First draw the bullet left margin
        painter.setPen(palette.color(QPalette::Text));
        painter.drawLine( bulletLineX_, 0, bulletLineX_, viewport()->height() );
        painter.fillRect( 0, 0, bulletLineX_, viewport()->height(), Qt::darkGray );

        // Then draw each line
        for (int i = firstLine; i <= lastLine; i++) {
            // Position in pixel of the base line of the line to print
            const int yPos = (i-firstLine) * fontHeight;
            const int xPos = bulletLineX_ + 2;

            // string to print, cut to fit the length and position of the view
            const QString cutLine = lines[i - firstLine].mid( firstCol, firstCol+nbCols );

            if ( selection_.isLineSelected( i ) ) {
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

            // Is there something selected in the line?
            int start_col, end_col;
            if ( selection_.getPortionForLine( i, &start_col, &end_col ) ) {
                // We use the LineDrawer, with three chunks
                // (before selection, selection and after selection)
                LineDrawer lineDrawer( backColor );

                LOG(logDEBUG) << "Partial selection "
                    << start_col << " " << end_col;

                // Convert the chunk to screen based coordinates
                // (from line based)
                start_col -= firstCol;
                end_col   -= firstCol;
                lineDrawer.addChunk( 0, start_col, foreColor, backColor );
                lineDrawer.addChunk( start_col, end_col,
                        palette.color( QPalette::HighlightedText ),
                        palette.color( QPalette::Highlight ) );
                lineDrawer.addChunk( end_col, cutLine.length(),
                        foreColor, backColor );

                lineDrawer.draw( painter, xPos, yPos,
                        viewport()->width(), cutLine );
            }
            else {
                // We can draw the line in one go!
                painter.fillRect( xPos, yPos, viewport()->width(),
                        fontHeight, backColor );
                painter.setPen( foreColor );
                painter.drawText( xPos, yPos + fontAscent, cutLine );
            }

            // Then draw the bullet
            const int circleSize = 3;

            if ( isLineMatching( i ) )
                painter.setBrush( matchBulletBrush );
            else
                painter.setBrush( normalBulletBrush );
            painter.setPen( palette.color( QPalette::Text ) );
            painter.drawEllipse( bulletLineX_/2 - circleSize,
                    yPos + (fontHeight / 2) - circleSize,
                    circleSize * 2, circleSize * 2 );
        }
    }
    LOG(logDEBUG4) << "End of repaint";
}

//
// Public functions
//

void AbstractLogView::updateData()
{
    // Check the top Line is within range
    if ( firstLine >= logData->getNbLine() ) {
        firstLine = 0;
        firstCol = 0;
        verticalScrollBar()->setValue( 0 );
        horizontalScrollBar()->setValue( 0 );
    }

    // Crop selection if it become out of range
    selection_.crop( logData->getNbLine() - 1 );

    // Adapt the scroll bars to the new content
    verticalScrollBar()->setRange( 0, logData->getNbLine()-1 );
    const int hScrollMaxValue = ( logData->getMaxLength() - getNbVisibleCols() + 1 ) > 0 ?
        ( logData->getMaxLength() - getNbVisibleCols() + 1 ) : 0;
    horizontalScrollBar()->setRange( 0, hScrollMaxValue );

    lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );

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

int AbstractLogView::getTopLine() const
{
    return firstLine;
}

QString AbstractLogView::getSelection() const
{
    return logData->getLineString( selection_.getLines() );
}

// Select the line and ensure it is visible on the screen, scrolling if not.
void AbstractLogView::displayLine( int line )
{
    LOG(logDEBUG) << "displayLine " << line << " nbLines: " << logData->getNbLine();

    selection_.selectLine( line );

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
    return viewport()->width()/fm.width('i') + 1;
}

// Converts the mouse x, y coordinates to the line number in the file
int AbstractLogView::convertCoordToLine(int yPos) const
{
    QFontMetrics fm = fontMetrics();
    int line = firstLine + yPos/fm.height();

    return line;
}

// Converts the mouse x, y coordinates to the char coordinates (in the file)
// This function ensure the pos exists in the file.
QPoint AbstractLogView::convertCoordToFilePos( const QPoint& pos ) const
{
    QFontMetrics fm = fontMetrics();
    int line = firstLine + pos.y() / fm.height();
    if ( line < 0 )
        line = 0;
    if ( line > logData->getNbLine() )
        line = logData->getNbLine();

    // FIXME, only works with fixed width fonts!!
    int column = firstCol + ( pos.x() - leftMarginPx_ ) / fm.width('i');
    if ( column < 0 )
        column = 0;
    if ( column > logData->getLineLength( line ) )
        column = logData->getLineLength( line );

    QPoint point( column, line );

    return point;
}

// Move the selection up and down by the passed number of lines
void AbstractLogView::moveSelection( int y )
{
    LOG(logDEBUG) << "AbstractLogView::moveSelection y=" << y;

    int new_line = selection_.getLines();

    if ( new_line == -1 )
        new_line = 0;
    else
        new_line += y;

    if ( new_line >= logData->getNbLine() )
        new_line = logData->getNbLine() - 1;
    if ( new_line < 0 )
        new_line = 0;

    displayLine( new_line );
}

// Select the first line and jump there
void AbstractLogView::jumpToTop()
{
    const int new_top_line = 0;

    selection_.selectLine( new_top_line );

    // This will also trigger a scrollContents event
    verticalScrollBar()->setValue( new_top_line );
    update();       // in case the screen hasn't moved
}

// Select the last line and jump there
void AbstractLogView::jumpToBottom()
{
    const int new_top_line =
        qMax( logData->getNbLine() - getNbVisibleLines() + 1, 0LL );

    selection_.selectLine( logData->getNbLine() - 1 );

    // This will also trigger a scrollContents event
    verticalScrollBar()->setValue( new_top_line );
    update();       // in case the screen hasn't moved
}

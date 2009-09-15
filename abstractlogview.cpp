
#include <iostream>

#include <QFile>
#include <QRect>
#include <QPaintEvent>
#include <QPainter>
#include <QFontMetrics>
#include <QScrollBar>

#include "log.h"

#include "common.h"
#include "logmainview.h"

AbstractLogView::AbstractLogView(QWidget* parent) : QAbstractScrollArea(parent)
{
    logData = NULL;
    // Create the viewport QWidget
    setViewport(0);
}

/*
 * Received events
 */
void AbstractLogView::mousePressEvent( QMouseEvent* mouseEvent )
{
    // Selection implementation
    if ( mouseEvent->button() == Qt::LeftButton )
    {
        int line = convertCoordToLine(mouseEvent->y());
        selectedLine = line;

        emit newSelection( line );
    }
}

void AbstractLogView::resizeEvent(QResizeEvent* resizeEvent)
{
    if ( logData == NULL )
        return;

    LOG(logDEBUG) << "resizeEvent received";

    // Calculate the index of the last line shown
    lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );

    // Update the scroll bars
    verticalScrollBar()->setPageStep( getNbVisibleLines() );
}

void AbstractLogView::scrollContentsBy( int dx, int dy )
{
    LOG(logDEBUG) << "scrollContentsBy received";

    firstLine -= dy;
    lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );

    update();
}

void AbstractLogView::paintEvent(QPaintEvent* paintEvent)
{
    QRect invalidRect = paintEvent->rect();
    if ( (invalidRect.isEmpty()) || (logData == NULL) )
        return;

    LOG(logDEBUG) << "paintEvent received, firstLine=" << firstLine
        << " lastLine=" << lastLine;

    {
        // Repaint the viewport
        QPainter painter(viewport());
        int fontHeight = painter.fontMetrics().height();
        int fontAscent = painter.fontMetrics().ascent();

        painter.fillRect(invalidRect, palette().color(QPalette::Window)); // Check if necessary
        painter.setPen( palette().color(QPalette::Text) );
        for (int i = firstLine; i < lastLine; i++) {
            int yPos = (i-firstLine) * fontHeight;
            if ( i == selectedLine )
            {
                painter.fillRect( 0, yPos, viewport()->width(), fontHeight,
                        palette().color(QPalette::Highlight) );
                painter.setPen( palette().color(QPalette::HighlightedText) );
                painter.drawText( 0, yPos + fontAscent, logData->getLineString(i) );
                painter.setPen( palette().color(QPalette::Text) );
            }
            else
                painter.drawText( 0, yPos + fontAscent, logData->getLineString(i) );
        }
    }

}

/*
 * Public functions
 */
void AbstractLogView::updateData(const AbstractLogData* newLogData)
{
    // Save the new data
    logData = newLogData;

    // Adapt the view to the new content
    LOG(logDEBUG) << "Now adapting the content";
    verticalScrollBar()->setValue( 0 );
    verticalScrollBar()->setRange( 0, logData->getNbLine()-1 );
    firstLine = 0;
    lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );
    selectedLine = -1;
    update();
}

/// @brief Display the particular line passed at the top of the display
void AbstractLogView::displayLine( int line )
{
    LOG(logDEBUG) << "displayLine " << line << " nbLines: " << logData->getNbLine();

    firstLine = line;
    selectedLine = line;
    //verticalScrollBar()->setValue( line );
    lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );

    update();
}

/*
 * Private functions
 */
int AbstractLogView::getNbVisibleLines()
{
    QFontMetrics fm = fontMetrics();
    return height()/fm.height() + 1;
}

/// @brief Convert the x, y coordinates to the line number in the file
int AbstractLogView::convertCoordToLine(int yPos)
{
    QFontMetrics fm = fontMetrics();
    int line = firstLine + yPos/fm.height();

    return line;
}

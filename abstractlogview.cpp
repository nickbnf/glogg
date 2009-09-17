
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
        << " lastLine=" << lastLine;

    {
        // Repaint the viewport
        QPainter painter(viewport());
        const int fontHeight = painter.fontMetrics().height();
        const int fontAscent = painter.fontMetrics().ascent();
        const int nbCols = getNbVisibleCols();

        painter.fillRect(invalidRect, palette().color(QPalette::Window)); // Check if necessary
        painter.setPen( palette().color(QPalette::Text) );
        for (int i = firstLine; i < lastLine; i++) {
            const int yPos = (i-firstLine) * fontHeight;
            const QString cutLine = logData->getLineString( i ).mid( firstCol, firstCol+nbCols );

            if ( i == selectedLine )
            {
                painter.fillRect( 0, yPos, viewport()->width(), fontHeight,
                        palette().color(QPalette::Highlight) );
                painter.setPen( palette().color(QPalette::HighlightedText) );
                painter.drawText( 0, yPos + fontAscent, cutLine );
                painter.setPen( palette().color(QPalette::Text) );
            }
            else
                painter.drawText( 0, yPos + fontAscent, cutLine );
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
    const int hScrollMaxValue = ( logData->getMaxLength() - getNbVisibleCols() + 1 ) > 0 ?
        ( logData->getMaxLength() - getNbVisibleCols() + 1 ) : 0;
    LOG(logDEBUG) << "getMaxLength=" << logData->getMaxLength();
    LOG(logDEBUG) << "getNbVisibleCols=" << getNbVisibleCols();
    LOG(logDEBUG) << "hScrollMaxValue=" << hScrollMaxValue;
    horizontalScrollBar()->setValue( 0 );
    horizontalScrollBar()->setRange( 0,  hScrollMaxValue );

    firstLine = 0;
    lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );
    firstCol = 0;
    selectedLine = -1;

    update();
}

/// @brief Display the particular line passed at the top of the display
void AbstractLogView::displayLine( int line )
{
    LOG(logDEBUG) << "displayLine " << line << " nbLines: " << logData->getNbLine();

    selectedLine = line;
    verticalScrollBar()->setValue( line );
}

/*
 * Private functions
 */
int AbstractLogView::getNbVisibleLines() const
{
    QFontMetrics fm = fontMetrics();
    return viewport()->height()/fm.height() + 1;
}

int AbstractLogView::getNbVisibleCols() const
{
    QFontMetrics fm = fontMetrics();
    return viewport()->width()/fm.width('w') + 1;
}

/// @brief Convert the x, y coordinates to the line number in the file
int AbstractLogView::convertCoordToLine(int yPos) const
{
    QFontMetrics fm = fontMetrics();
    int line = firstLine + yPos/fm.height();

    return line;
}

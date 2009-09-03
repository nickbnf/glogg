#include <iostream>

#include <QRect>
#include <QPaintEvent>
#include <QPainter>
#include <QFontMetrics>
#include <QScrollBar>

#include "log.h"

#include "common.h"
#include "filteredview.h"

FilteredView::FilteredView(QWidget* parent) : QAbstractScrollArea(parent)
{
    logFilteredData = NULL;
    // Create the viewport QWidget
    setViewport(0);
}

void FilteredView::resizeEvent(QResizeEvent* resizeEvent)
{
    if ( logFilteredData == NULL )
        return;

    LOG(logDEBUG) << "resizeEvent received";

    // Calculate the index of the last line shown
    lastLine = min2( logFilteredData->getNbLine(), firstLine + getNbVisibleLines() );

    // Update the scroll bars
    verticalScrollBar()->setPageStep( getNbVisibleLines() );
}

void FilteredView::scrollContentsBy( int dx, int dy )
{
    LOG(logDEBUG) << "scrollContentsBy received";

    if (logFilteredData == NULL)
        return;

    firstLine -= dy;
    lastLine = min2( logFilteredData->getNbLine(), firstLine + getNbVisibleLines() );

    update();
}

void FilteredView::paintEvent(QPaintEvent* paintEvent)
{
    QRect invalidRect = paintEvent->rect();
    if ( (invalidRect.isEmpty()) || (logFilteredData == NULL) )
        return;

    LOG(logDEBUG) << "paintEvent received, firstLine=" << firstLine
        << " lastLine=" << lastLine;

    {
        // Repaint the viewport
        QPainter painter(viewport());
        int fontHeight = painter.fontMetrics().height();
        // painter.fillRect(invalidRect, Qt::white); // useless...
        painter.setPen( Qt::black );
        for (int i = firstLine; i < lastLine; i++) {
            int yPos = (i-firstLine + 1) * fontHeight;
            painter.drawText(0, yPos, logFilteredData->getLineString(i));
        }
    }

}

int FilteredView::getNbVisibleLines()
{
    QFontMetrics fm = fontMetrics();
    return height()/fm.height() + 1;
}

void FilteredView::updateData(const LogFilteredData* newLogFilteredData)
{
    // Save the new data
    logFilteredData = newLogFilteredData;

    // Adapt the view to the new content
    LOG(logDEBUG) << "Now adapting the content";
    verticalScrollBar()->setValue( 0 );
    verticalScrollBar()->setRange( 0, logFilteredData->getNbLine()-1 );
    firstLine = 0;
    lastLine = min2( logFilteredData->getNbLine(), firstLine + getNbVisibleLines() );
    update();
}

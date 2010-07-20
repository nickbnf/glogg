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

#include <QApplication>
#include <QClipboard>
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
#include "quickfind.h"
#include "quickfindpattern.h"


LineChunk::LineChunk( int first_col, int last_col, ChunkType type )
{
    LOG(logDEBUG) << "new LineChunk: " << first_col << " " << last_col;

    start_ = first_col;
    end_   = last_col;
    type_  = type;
}

QList<LineChunk> LineChunk::select( int sel_start, int sel_end ) const
{
    QList<LineChunk> list;

    if ( ( sel_start < start_ ) && ( sel_end < start_ ) ) {
        // Selection BEFORE this chunk: no change
        list << LineChunk( *this );
    }
    else if ( sel_start > end_ ) {
        // Selection AFTER this chunk: no change
        list << LineChunk( *this );
    }
    else /* if ( ( sel_start >= start_ ) && ( sel_end <= end_ ) ) */
    {
        // We only want to consider what's inside THIS chunk
        sel_start = qMax( sel_start, start_ );
        sel_end   = qMin( sel_end, end_ );

        if ( sel_start > start_ )
            list << LineChunk( start_, sel_start - 1, type_ );
        list << LineChunk( sel_start, sel_end, Selected );
        if ( sel_end < end_ )
            list << LineChunk( sel_end + 1, end_, type_ );
    }

    return list;
}

inline void LineDrawer::addChunk( int first_col, int last_col,
        QColor fore, QColor back )
{
    if ( first_col < 0 )
        first_col = 0;
    int length = last_col - first_col + 1;
    if ( length > 0 ) {
        list << Chunk ( first_col, length, fore, back );
    }
}

inline void LineDrawer::addChunk( const LineChunk& chunk,
        QColor fore, QColor back )
{
    int first_col = chunk.start();
    int last_col  = chunk.end();

    addChunk( first_col, last_col, fore, back );
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

    if ( blank_width > 0 )
        painter.fillRect( xPos, yPos, blank_width, fontHeight, backColor_ );
}

// Looks better with an odd value
const int AbstractLogView::bulletLineX_ = 11;
const int AbstractLogView::leftMarginPx_ = bulletLineX_ + 2;

AbstractLogView::AbstractLogView(const AbstractLogData* newLogData,
        const QuickFindPattern* const quickFindPattern, QWidget* parent) :
    QAbstractScrollArea( parent ),
    selectionStartPos_(),
    selectionCurrentEndPos_(),
    autoScrollTimer_(),
    selection_(),
    quickFindPattern_( quickFindPattern ),
    quickFind_( newLogData, &selection_, quickFindPattern )
{
    logData           = newLogData;

    // Create the viewport QWidget
    setViewport(0);
    setAttribute(Qt::WA_StaticContents);  // Does it work?

    selectionStarted_ = false;

    firstLine = 0;
    lastLine = 0;
    firstCol = 0;

    // Fonts
    useFixedFont_ = false;
    charWidth_ = 1;
    charHeight_ = 1;

    // Signals
    connect( quickFindPattern_, SIGNAL( patternUpdated() ),
            this, SLOT ( update() ) );
}

AbstractLogView::~AbstractLogView()
{
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
        selectionCurrentEndPos_ = selectionStartPos_;
    }
}

void AbstractLogView::mouseMoveEvent( QMouseEvent* mouseEvent )
{
    // Selection implementation
    if ( selectionStarted_ )
    {
        QPoint thisEndPos = convertCoordToFilePos( mouseEvent->pos() );
        if ( thisEndPos != selectionCurrentEndPos_ )
        {
            // Are we on a different line?
            if ( selectionStartPos_.y() != thisEndPos.y() )
            {
                if ( thisEndPos.y() != selectionCurrentEndPos_.y() )
                {
                    // This is a 'range' selection
                    selection_.selectRange( selectionStartPos_.y(),
                            thisEndPos.y() );
                    update();
                }
            }
            // So we are on the same line. Are we moving horizontaly?
            else if ( thisEndPos.x() != selectionCurrentEndPos_.x() )
            {
                // This is a 'portion' selection
                selection_.selectPortion( thisEndPos.y(),
                        selectionStartPos_.x(), thisEndPos.x() );
                update();
            }
            // On the same line, and moving vertically then
            else
            {
                // This is a 'line' selection
                selection_.selectLine( thisEndPos.y() );
                update();
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
    selectionStarted_ = false;
    if ( autoScrollTimer_.isActive() )
        autoScrollTimer_.stop();
    updateGlobalSelection();
}

void AbstractLogView::mouseDoubleClickEvent( QMouseEvent* mouseEvent )
{
    if ( mouseEvent->button() == Qt::LeftButton )
    {
        const QPoint pos = convertCoordToFilePos( mouseEvent->pos() );
        selectWordAtPosition( pos );
    }
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
    else if ( keyEvent->key() == Qt::Key_Home )
        jumpToStartOfLine();
    else if ( keyEvent->key() == Qt::Key_End )
        jumpToRightOfScreen();
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
            case '0':
                jumpToStartOfLine();
                break;
            case '$':
                jumpToEndOfLine();
                break;
            case 'g':
                jumpToTop();
                break;
            case 'G':
                jumpToBottom();
                break;
            case 'n':
                searchNext();
                break;
            default:
                keyEvent->ignore();
        }
    }

    if ( !keyEvent->isAccepted() )
        QAbstractScrollArea::keyPressEvent( keyEvent );
}

void AbstractLogView::resizeEvent( QResizeEvent* )
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
            const QString cutLine = lines[i - firstLine].mid( firstCol, nbCols );

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
            int sel_start, sel_end;
            bool isSelection =
                selection_.getPortionForLine( i, &sel_start, &sel_end );
            // Has the line got elements to be highlighted
            QList<QuickFindMatch> qfMatchList;
            bool isMatch =
                quickFindPattern_->matchLine( cutLine, qfMatchList );

            if ( isSelection || isMatch ) {
                // We use the LineDrawer and its chunks because the
                // line has to be somehow highlighted
                LineDrawer lineDrawer( backColor );

                // First we create a list of chunks with the highlights
                QList<LineChunk> chunkList;
                int column = 0; // Current column in line space
                foreach( const QuickFindMatch match, qfMatchList ) {
                    int start = match.startColumn();
                    if ( start > column )
                        chunkList << LineChunk( column, start - 1, LineChunk::Normal );
                    column = start + match.length() - 1;
                    chunkList << LineChunk( start, column, LineChunk::Highlighted );
                    column++;
                }
                chunkList << LineChunk( column, cutLine.length() - 1, LineChunk::Normal );

                // Then we add the selection if needed
                QList<LineChunk> newChunkList;
                if ( isSelection ) {
                    sel_start -= firstCol; // coord in line space
                    sel_end   -= firstCol;

                    foreach ( const LineChunk chunk, chunkList ) {
                        newChunkList << chunk.select( sel_start, sel_end );
                    }
                }
                else
                    newChunkList = chunkList;

                foreach ( const LineChunk chunk, newChunkList ) {
                    // Select the colours
                    QColor fore;
                    QColor back;
                    switch ( chunk.type() ) {
                        case LineChunk::Normal:
                            fore = foreColor;
                            back = backColor;
                            break;
                        case LineChunk::Highlighted:
                            fore = QColor( "black" );
                            back = QColor( "yellow" );
                            // fore = highlightForeColor;
                            // back = highlightBackColor;
                            break;
                        case LineChunk::Selected:
                            fore = palette.color( QPalette::HighlightedText ),
                            back = palette.color( QPalette::Highlight );
                            break;
                    }
                    lineDrawer.addChunk ( chunk, fore, back );
                }

                lineDrawer.draw( painter, xPos, yPos,
                        viewport()->width(), cutLine );
            }
            else {
                // Nothing to be highlighted, we print the whole line!
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

void AbstractLogView::searchNext()
{
    LOG(logDEBUG) << "AbstractLogView::searchNext";

    int line = quickFind_.searchNext();
    if ( line >= 0 ) {
        LOG(logDEBUG) << "searchNext " << line;
        displayLine( line );
    }
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
    QFontMetrics fm = fontMetrics();

    charHeight_ = fm.height();
    // Determine if the font is monospace (to enable various optimisations)
    useFixedFont_ = true;
    charWidth_ = fm.width('i');
    for ( int i = 0x20; i < 0x7F; i++ )
        if ( fm.width( QLatin1Char( i ) ) != charWidth_ ) {
            useFixedFont_ = false;
            break;
        }
    // FIXME: seems to be wrong on Qt/Mac
    LOG(logDEBUG) << "useFixedFont_=" << useFixedFont_;

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
    return selection_.getSelectedText( logData );
}

void AbstractLogView::selectAndDisplayLine( int line )
{
    selection_.selectLine( line );
    displayLine( line );
}

//
// Private functions
//

// Returns the number of lines visible in the viewport
int AbstractLogView::getNbVisibleLines() const
{
    return viewport()->height() / charHeight_ + 1;
}

// Returns the number of columns visible in the viewport
int AbstractLogView::getNbVisibleCols() const
{
    return ( viewport()->width() - leftMarginPx_ ) / charWidth_ + 1;
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
    int line = firstLine + pos.y() / charHeight_;
    if ( line >= logData->getNbLine() )
        line = logData->getNbLine() - 1;
    if ( line < 0 )
        line = 0;

    int column = 0;
    // Determine column in screen space
    if ( useFixedFont_ || ( pos.x()-leftMarginPx_ < 0 ) )
        column = ( pos.x() - leftMarginPx_ ) / charWidth_;
    else {
        QFontMetrics fm = fontMetrics();
        QString this_line = logData->getLineString( line );
        const int length = this_line.length();
        for ( ; column < length; column++ ) {
            if ( ( fm.width( this_line.mid(firstCol, column) )
                        + leftMarginPx_ ) > pos.x() ) {
                column--;
                break;
            }
        }
    }
    // Now convert it to file space
    column += firstCol;

    if ( column >= logData->getLineLength( line ) )
        column = logData->getLineLength( line ) - 1;
    if ( column < 0 )
        column = 0;

    QPoint point( column, line );

    return point;
}

// Makes the widget adjust itself to display the passed line.
// Doing so, it will throw itself a scrollContents event.
void AbstractLogView::displayLine( int line )
{
    LOG(logDEBUG) << "displayLine " << line << " nbLines: " << logData->getNbLine();

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

// Move the selection up and down by the passed number of lines
void AbstractLogView::moveSelection( int delta )
{
    LOG(logDEBUG) << "AbstractLogView::moveSelection delta=" << delta;

    QList<int> selection = selection_.getLines();
    int new_line;

    // If nothing is selected, do as if line -1 was.
    if ( selection.isEmpty() )
        selection.append( -1 );

    if ( delta < 0 )
        new_line = selection.first() + delta;
    else
        new_line = selection.last() + delta;

    if ( new_line < 0 )
        new_line = 0;
    else if ( new_line >= logData->getNbLine() )
        new_line = logData->getNbLine() - 1;

    // Select and display the new line
    selection_.selectLine( new_line );
    displayLine( new_line );
}

// Make the start of the lines visible
void AbstractLogView::jumpToStartOfLine()
{
    horizontalScrollBar()->setValue( 0 );
}

// Make the end of the lines in the selection visible
void AbstractLogView::jumpToEndOfLine()
{
    QList<int> selection = selection_.getLines();

    // Search the longest line in the selection
    int max_length = 0;
    foreach ( int line, selection ) {
        int length = logData->getLineLength( line );
        if ( length > max_length )
            max_length = length;
    }

    horizontalScrollBar()->setValue( max_length - getNbVisibleCols() );
}

// Make the end of the lines on the screen visible
void AbstractLogView::jumpToRightOfScreen()
{
    QList<int> selection = selection_.getLines();

    // Search the longest line on screen
    int max_length = 0;
    for ( int i = firstLine; i <= ( firstLine + getNbVisibleLines() ); i++ ) {
        int length = logData->getLineLength( i );
        if ( length > max_length )
            max_length = length;
    }

    horizontalScrollBar()->setValue( max_length - getNbVisibleCols() );
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

// Returns whether the character pass is a 'word' character
inline bool AbstractLogView::isCharWord( char c )
{
    if ( ( ( c >= 'A' ) && ( c <= 'Z' ) ) ||
         ( ( c >= 'a' ) && ( c <= 'z' ) ) ||
         ( ( c >= '0' ) && ( c <= '9' ) ) ||
         ( ( c == '_' ) ) )
        return true;
    else
        return false;
}

// Select the word under the given position
void AbstractLogView::selectWordAtPosition( const QPoint& pos )
{
    const int x = pos.x();
    const QString line = logData->getLineString( pos.y() );

    if ( isCharWord( line[x].toLatin1() ) ) {
        // Search backward for the first character in the word
        int currentPos = x;
        for ( ; currentPos > 0; currentPos-- )
            if ( ! isCharWord( line[currentPos].toLatin1() ) )
            {
                currentPos++;
                break;
            }
        int start = currentPos;

        // Now search for the end
        currentPos = x;
        for ( ; currentPos < line.length() - 1; currentPos++ )
            if ( ! isCharWord( line[currentPos].toLatin1() ) )
            {
                currentPos--;
                break;
            }
        int end = currentPos;

        selection_.selectPortion( pos.y(), start, end );
        updateGlobalSelection();
        update();
    }
}

// Update the system global (middle click) selection (X11 only)
void AbstractLogView::updateGlobalSelection()
{
    static QClipboard* const clipboard = QApplication::clipboard();

    // Updating it only for "non-trivial" (range or portion) selections
    if ( ! selection_.isSingleLine() )
        clipboard->setText( selection_.getSelectedText( logData ),
                QClipboard::Selection );
}

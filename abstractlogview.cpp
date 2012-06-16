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
#include <QMenu>
#include <QAction>

#include "log.h"

#include "persistentinfo.h"
#include "filterset.h"
#include "logmainview.h"
#include "quickfind.h"
#include "quickfindpattern.h"
#include "overview.h"
#include "configuration.h"


LineChunk::LineChunk( int first_col, int last_col, ChunkType type )
{
    // LOG(logDEBUG) << "new LineChunk: " << first_col << " " << last_col;

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
        // LOG(logDEBUG) << "Chunk: " << chunk.start() << " " << chunk.length();
        QString cutline = line.mid( chunk.start(), chunk.length() );
        // Note that fm.width is not the same as len * maxWidth due to
        // bearings.  We could use the latter and take bearings into account,
        // but perhaps the font metics already makes width() efficient for
        // fixed width fonts.
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

const int DigitsBuffer::timeout_ = 2000;

DigitsBuffer::DigitsBuffer() : QObject()
{
}

void DigitsBuffer::reset()
{
    LOG(logDEBUG) << "DigitsBuffer::reset()";

    timer_.stop();
    digits_.clear();
}

void DigitsBuffer::add( char character )
{
    LOG(logDEBUG) << "DigitsBuffer::add()";

    digits_.append( QChar( character ) );
    timer_.start( timeout_ , this );
}

int DigitsBuffer::content()
{
    int result = digits_.toInt();
    reset();

    return result;
}

void DigitsBuffer::timerEvent( QTimerEvent* event )
{
    if ( event->timerId() == timer_.timerId() ) {
        reset();
    }
    else {
        QObject::timerEvent( event );
    }
}

// Graphic parameters

// Looks better with an odd value
const int AbstractLogView::bulletLineX_ = 11;
const int AbstractLogView::leftMarginPx_ = bulletLineX_ + 2;

const int AbstractLogView::OVERVIEW_WIDTH = 27;

AbstractLogView::AbstractLogView(const AbstractLogData* newLogData,
        const QuickFindPattern* const quickFindPattern, QWidget* parent) :
    QAbstractScrollArea( parent ),
    selectionStartPos_(),
    selectionCurrentEndPos_(),
    autoScrollTimer_(),
    selection_(),
    quickFindPattern_( quickFindPattern ),
    quickFind_( newLogData, &selection_, quickFindPattern ),
    overviewWidget_( this )
{
    logData = newLogData;

    followMode_ = false;

    selectionStarted_ = false;
    markingClickInitiated_ = false;

    firstLine = 0;
    lastLine = 0;
    firstCol = 0;

    overview_ = NULL;

    // Fonts
    charWidth_ = 1;
    charHeight_ = 1;

    // Create the viewport QWidget
    setViewport( 0 );

    setAttribute( Qt::WA_StaticContents );  // Does it work?

    // Init the popup menu
    createMenu();

    // Signals
    connect( quickFindPattern_, SIGNAL( patternUpdated() ),
            this, SLOT ( handlePatternUpdated() ) );
    connect( verticalScrollBar(), SIGNAL( sliderMoved( int ) ),
            this, SIGNAL( followDisabled() ) );
    connect( &quickFind_, SIGNAL( notify( const QString& ) ),
            this, SIGNAL( notifyQuickFind( const QString& ) ) );
    connect( &quickFind_, SIGNAL( clearNotification() ),
            this, SIGNAL( clearQuickFindNotification() ) );
    connect( &overviewWidget_, SIGNAL( lineClicked ( int ) ),
            this, SIGNAL( followDisabled() ) );
    connect( &overviewWidget_, SIGNAL( lineClicked ( int ) ),
            this, SLOT( jumpToLine( int ) ) );
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
    static Configuration& config = Persistent<Configuration>( "settings" );

    // Selection implementation
    if ( mouseEvent->button() == Qt::LeftButton )
    {
        int line = convertCoordToLine( mouseEvent->y() );

        if ( mouseEvent->x() < leftMarginPx_ ) {
            // Mark a line if it is clicked in the left margin
            // (only if click and release in the same area)
            markingClickInitiated_ = true;
            markingClickLine_ = line;
        }
        else {
            // Select the line, and start a selection
            if ( line < logData->getNbLine() ) {
                selection_.selectLine( line );
                emit updateLineNumber( line );
                emit newSelection( line );
            }

            // Remember the click in case we're starting a selection
            selectionStarted_ = true;
            selectionStartPos_ = convertCoordToFilePos( mouseEvent->pos() );
            selectionCurrentEndPos_ = selectionStartPos_;
        }
    }
    else if ( mouseEvent->button() == Qt::RightButton )
    {
        // Prepare the popup depending on selection type
        if ( selection_.isSingleLine() ) {
            copyAction_->setText( "&Copy this line" );
        }
        else {
            copyAction_->setText( "&Copy" );
            copyAction_->setStatusTip( tr("Copy the selection") );
        }

        if ( selection_.isPortion() ) {
            findNextAction_->setEnabled( true );
            findPreviousAction_->setEnabled( true );
            addToSearchAction_->setEnabled( true );
        }
        else {
            findNextAction_->setEnabled( false );
            findPreviousAction_->setEnabled( false );
            addToSearchAction_->setEnabled( false );
        }

        // "Add to search" only makes sense in regexp mode
        if ( config.mainRegexpType() != ExtendedRegexp )
            addToSearchAction_->setEnabled( false );

        // Display the popup (blocking)
        popupMenu_->exec( QCursor::pos() );
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
                    emit updateLineNumber( thisEndPos.y() );
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
                emit updateLineNumber( thisEndPos.y() );
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

void AbstractLogView::mouseReleaseEvent( QMouseEvent* mouseEvent )
{
    if ( markingClickInitiated_ ) {
        markingClickInitiated_ = false;
        int line = convertCoordToLine( mouseEvent->y() );
        if ( line == markingClickLine_ )
            emit markLine( line );
    }
    else {
        selectionStarted_ = false;
        if ( autoScrollTimer_.isActive() )
            autoScrollTimer_.stop();
        updateGlobalSelection();
    }
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
        const char character = (keyEvent->text())[0].toAscii();

        if ( ( character >= '0' ) && ( character <= '9' ) ) {
            // Adds the digit to the timed buffer
            digitsBuffer_.add( character );
        }
        else {
            switch ( (keyEvent->text())[0].toAscii() ) {
                case 'j':
                    {
                        int delta = qMax( 1, digitsBuffer_.content() );
                        emit followDisabled();
                        //verticalScrollBar()->triggerAction(
                        //QScrollBar::SliderSingleStepAdd);
                        moveSelection( delta );
                        break;
                    }
                case 'k':
                    {
                        int delta = qMin( -1, - digitsBuffer_.content() );
                        emit followDisabled();
                        //verticalScrollBar()->triggerAction(
                        //QScrollBar::SliderSingleStepSub);
                        moveSelection( delta );
                        break;
                    }
                case 'h':
                    horizontalScrollBar()->triggerAction(
                            QScrollBar::SliderSingleStepSub);
                    break;
                case 'l':
                    horizontalScrollBar()->triggerAction(
                            QScrollBar::SliderSingleStepAdd);
                    break;
                case '0':
                    jumpToStartOfLine();
                    break;
                case '$':
                    jumpToEndOfLine();
                    break;
                case 'g':
                    {
                        int newLine = qMax( 0, digitsBuffer_.content() - 1 );
                        if ( newLine >= logData->getNbLine() )
                            newLine = logData->getNbLine() - 1;
                        emit followDisabled();
                        selectAndDisplayLine( newLine );
                        emit updateLineNumber( newLine );
                        break;
                    }
                case 'G':
                    emit followDisabled();
                    selection_.selectLine( logData->getNbLine() - 1 );
                    emit updateLineNumber( logData->getNbLine() - 1 );
                    jumpToBottom();
                    break;
                case 'n':
                    searchNext();
                    break;
                case 'N':
                    searchPrevious();
                    break;
                case '*':
                    // Use the selected 'word' and search forward
                    findNextSelected();
                    break;
                case '#':
                    // Use the selected 'word' and search backward
                    findPreviousSelected();
                    break;
                default:
                    keyEvent->ignore();
            }
        }
    }

    if ( !keyEvent->isAccepted() )
        QAbstractScrollArea::keyPressEvent( keyEvent );
}

void AbstractLogView::wheelEvent( QWheelEvent* wheelEvent )
{
    emit followDisabled();

    QAbstractScrollArea::wheelEvent( wheelEvent );
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
    lastLine = qMin( logData->getNbLine(), firstLine + getNbVisibleLines() );

    // Update the overview if we have one
    if ( overview_ != NULL )
        overview_->updateCurrentPosition( firstLine, lastLine );

    // Redraw
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
        const int fontHeight = charHeight_;
        const int fontAscent = painter.fontMetrics().ascent();
        const int nbCols = getNbVisibleCols();
        const QPalette& palette = viewport()->palette();
        const FilterSet& filterSet = Persistent<FilterSet>( "filterSet" );
        QColor foreColor, backColor;

        static const QBrush normalBulletBrush = QBrush( Qt::white );
        static const QBrush matchBulletBrush = QBrush( Qt::red );
        static const QBrush markBrush = QBrush( "dodgerblue" );

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
        const QStringList lines = logData->getExpandedLines( firstLine, lastLine - firstLine + 1 );

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
            const QString line = lines[i - firstLine];
            const QString cutLine = line.mid( firstCol, nbCols );

            if ( selection_.isLineSelected( i ) ) {
                // Reverse the selected line
                foreColor = palette.color( QPalette::HighlightedText );
                backColor = palette.color( QPalette::Highlight );
                painter.setPen(palette.color(QPalette::Text));
            }
            else if ( filterSet.matchLine( logData->getLineString( i ),
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
                quickFindPattern_->matchLine( line, qfMatchList );

            if ( isSelection || isMatch ) {
                // We use the LineDrawer and its chunks because the
                // line has to be somehow highlighted
                LineDrawer lineDrawer( backColor );

                // First we create a list of chunks with the highlights
                QList<LineChunk> chunkList;
                int column = 0; // Current column in line space
                foreach( const QuickFindMatch match, qfMatchList ) {
                    int start = match.startColumn() - firstCol;
                    int end = start + match.length();
                    // Ignore matches that are *completely* outside view area
                    if ( ( start < 0 && end < 0 ) || start >= nbCols )
                        continue;
                    if ( start > column )
                        chunkList << LineChunk( column, start - 1, LineChunk::Normal );
                    column = qMin( start + match.length() - 1, nbCols );
                    chunkList << LineChunk( qMax( start, 0 ), column,
                                            LineChunk::Highlighted );
                    column++;
                }
                if ( column <= cutLine.length() - 1 )
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
            painter.setPen( palette.color( QPalette::Text ) );
            const int circleSize = 3;
            const int arrowHeight = 4;
            const int middleXLine = bulletLineX_ / 2;
            const int middleYLine = yPos + (fontHeight / 2);

            const LineType line_type = lineType( i );
            if ( line_type == Marked ) {
                // A pretty arrow if the line is marked
                const QPoint points[7] = {
                    QPoint(1, middleYLine - 2),
                    QPoint(middleXLine, middleYLine - 2),
                    QPoint(middleXLine, middleYLine - arrowHeight),
                    QPoint(bulletLineX_ - 2, middleYLine),
                    QPoint(middleXLine, middleYLine + arrowHeight),
                    QPoint(middleXLine, middleYLine + 2),
                    QPoint(1, middleYLine + 2 ),
                };

                painter.setBrush( markBrush );
                painter.drawPolygon( points, 7 );
            }
            else {
                if ( lineType( i ) == Match )
                    painter.setBrush( matchBulletBrush );
                else
                    painter.setBrush( normalBulletBrush );
                painter.drawEllipse( middleXLine - circleSize,
                        middleYLine - circleSize,
                        circleSize * 2, circleSize * 2 );
            }
        } // For each line
    }
    LOG(logDEBUG4) << "End of repaint";
}

void AbstractLogView::setOverview( Overview* overview )
{
    overview_ = overview;
    refreshOverview();
    overviewWidget_.setOverview( overview );
}

void AbstractLogView::searchNext()
{
    searchForward();
}

void AbstractLogView::searchPrevious()
{
    searchBackward();
}

void AbstractLogView::searchForward()
{
    emit followDisabled();

    int line = quickFind_.searchForward();
    if ( line >= 0 ) {
        LOG(logDEBUG) << "searchForward " << line;
        displayLine( line );
        emit updateLineNumber( line );
    }
}

void AbstractLogView::searchBackward()
{
    emit followDisabled();

    int line = quickFind_.searchBackward();
    if ( line >= 0 ) {
        LOG(logDEBUG) << "searchBackward " << line;
        displayLine( line );
        emit updateLineNumber( line );
    }
}

void AbstractLogView::followSet( bool checked )
{
    followMode_ = checked;
    if ( checked )
        jumpToBottom();
}

void AbstractLogView::refreshOverview()
{
    // Create space for the Overview if needed
    if ( ( getOverview() != NULL ) && getOverview()->isVisible() ) {
        setViewportMargins( 0, 0, OVERVIEW_WIDTH, 0 );
        overviewWidget_.show();
    }
    else {
        setViewportMargins( 0, 0, 0, 0 );
        overviewWidget_.hide();
    }
}

// Reset the QuickFind when the pattern is changed.
void AbstractLogView::handlePatternUpdated()
{
    LOG(logDEBUG) << "AbstractLogView::handlePatternUpdated()";

    quickFind_.resetLimits();
    update();
}

// OR the current with the current search expression
void AbstractLogView::addToSearch()
{
    if ( selection_.isPortion() ) {
        LOG(logDEBUG) << "AbstractLogView::addToSearch()";
        emit addToSearch( selection_.getSelectedText( logData ) );
    }
    else {
        LOG(logERROR) << "AbstractLogView::addToSearch called for a wrong type of selection";
    }
}

// Find next occurence of the selected text (*)
void AbstractLogView::findNextSelected()
{
    // Use the selected 'word' and search forward
    if ( selection_.isPortion() ) {
        emit changeQuickFind(
                selection_.getSelectedText( logData ) );
        searchNext();
    }
}

// Find next previous of the selected text (#)
void AbstractLogView::findPreviousSelected()
{
    if ( selection_.isPortion() ) {
        emit changeQuickFind(
                selection_.getSelectedText( logData ) );
        searchPrevious();
    }
}

// Copy the selection to the clipboard
void AbstractLogView::copy()
{
    static QClipboard* clipboard = QApplication::clipboard();

    clipboard->setText( selection_.getSelectedText( logData ) );
}

//
// Public functions
//

void AbstractLogView::updateData()
{
    LOG(logDEBUG) << "AbstractLogView::updateData";

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

    lastLine = qMin( logData->getNbLine(), firstLine + getNbVisibleLines() );

    // Reset the QuickFind in case we have new stuff to search into
    quickFind_.resetLimits();

    if ( followMode_ )
        jumpToBottom();

    // Update the overview if we have one
    if ( overview_ != NULL )
        overview_->updateCurrentPosition( firstLine, lastLine );

    // Repaint!
    update();
}

void AbstractLogView::updateDisplaySize()
{
    // Font is assumed to be mono-space (is restricted by options dialog)
    QFontMetrics fm = fontMetrics();
    charHeight_ = fm.height();
    charWidth_ = fm.maxWidth();

    // Calculate the index of the last line shown
    lastLine = qMin( logData->getNbLine(), firstLine + getNbVisibleLines() );

    // Update the scroll bars
    verticalScrollBar()->setPageStep( getNbVisibleLines() );

    const int hScrollMaxValue = ( logData->getMaxLength() - getNbVisibleCols() + 1 ) > 0 ?
        ( logData->getMaxLength() - getNbVisibleCols() + 1 ) : 0;
    horizontalScrollBar()->setRange( 0, hScrollMaxValue );

    LOG(logDEBUG) << "viewport.width()=" << viewport()->width();
    LOG(logDEBUG) << "viewport.height()=" << viewport()->height();
    LOG(logDEBUG) << "width()=" << width();
    LOG(logDEBUG) << "height()=" << height();
    overviewWidget_.setGeometry( viewport()->width() + 2, 1,
            OVERVIEW_WIDTH - 1, viewport()->height() );
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
    emit followDisabled();
    selection_.selectLine( line );
    displayLine( line );
    emit updateLineNumber( line );
}

// The difference between this function and displayLine() is quite
// subtle: this one always jump, even if the line passed is visible.
void AbstractLogView::jumpToLine( int line )
{
    // Put the selected line in the middle if possible
    int newTopLine = line - ( getNbVisibleLines() / 2 );
    if ( newTopLine < 0 )
        newTopLine = 0;

    // This will also trigger a scrollContents event
    verticalScrollBar()->setValue( newTopLine );
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
    int line = firstLine + yPos / charHeight_;

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

    // Determine column in screen space and convert it to file space
    int column = firstCol + ( pos.x() - leftMarginPx_ ) / charWidth_;

    QString this_line = logData->getExpandedLineString( line );
    const int length = this_line.length();

    if ( column >= length )
        column = length - 1;
    if ( column < 0 )
        column = 0;

    LOG(logDEBUG) << "AbstractLogView::convertCoordToFilePos col="
        << column << " line=" << line;
    QPoint point( column, line );

    return point;
}

// Makes the widget adjust itself to display the passed line.
// Doing so, it will throw itself a scrollContents event.
void AbstractLogView::displayLine( int line )
{
    // If the line is already the screen
    if ( ( line >= firstLine ) &&
         ( line < ( firstLine + getNbVisibleLines() ) ) ) {
        // ... don't scroll and just repaint
        update();
    } else {
        jumpToLine( line );
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
    emit updateLineNumber( new_line );
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

// Jump to the first line
void AbstractLogView::jumpToTop()
{
    // This will also trigger a scrollContents event
    verticalScrollBar()->setValue( 0 );
    update();       // in case the screen hasn't moved
}

// Jump to the last line
void AbstractLogView::jumpToBottom()
{
    const int new_top_line =
        qMax( logData->getNbLine() - getNbVisibleLines() + 1, 0LL );

    // This will also trigger a scrollContents event
    verticalScrollBar()->setValue( new_top_line );
    update();       // in case the screen hasn't moved
}

// Returns whether the character passed is a 'word' character
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
    const QString line = logData->getExpandedLineString( pos.y() );

    if ( isCharWord( line[x].toLatin1() ) ) {
        // Search backward for the first character in the word
        int currentPos = x;
        for ( ; currentPos > 0; currentPos-- )
            if ( ! isCharWord( line[currentPos].toLatin1() ) )
                break;
        // Exclude the first char of the line if needed
        if ( ! isCharWord( line[currentPos].toLatin1() ) )
            currentPos++;
        int start = currentPos;

        // Now search for the end
        currentPos = x;
        for ( ; currentPos < line.length() - 1; currentPos++ )
            if ( ! isCharWord( line[currentPos].toLatin1() ) )
                break;
        // Exclude the last char of the line if needed
        if ( ! isCharWord( line[currentPos].toLatin1() ) )
            currentPos--;
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

// Create the pop-up menu
void AbstractLogView::createMenu()
{
    copyAction_ = new QAction( tr("&Copy"), this );
    // No text as this action title depends on the type of selection
    connect( copyAction_, SIGNAL(triggered()), this, SLOT(copy()) );

    // For '#' and '*', shortcuts doesn't seem to work but
    // at least it displays them in the menu, we manually handle those keys
    // as keys event anyway (in keyPressEvent).
    findNextAction_ = new QAction(tr("Find &next"), this);
    findNextAction_->setShortcut( Qt::Key_Asterisk );
    findNextAction_->setStatusTip( tr("Find the next occurence") );
    connect( findNextAction_, SIGNAL(triggered()),
            this, SLOT( findNextSelected() ) );

    findPreviousAction_ = new QAction( tr("Find &previous"), this );
    findPreviousAction_->setShortcut( tr("#")  );
    findPreviousAction_->setStatusTip( tr("Find the previous occurence") );
    connect( findPreviousAction_, SIGNAL(triggered()),
            this, SLOT( findPreviousSelected() ) );

    addToSearchAction_ = new QAction( tr("&Add to search"), this );
    addToSearchAction_->setStatusTip(
            tr("Add the selection to the current search") );
    connect( addToSearchAction_, SIGNAL( triggered() ),
            this, SLOT( addToSearch() ) );

    popupMenu_ = new QMenu( this );
    popupMenu_->addAction( copyAction_ );
    popupMenu_->addSeparator();
    popupMenu_->addAction( findNextAction_ );
    popupMenu_->addAction( findPreviousAction_ );
    popupMenu_->addAction( addToSearchAction_ );
}

/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2015 Nicolas Bonnefon
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

// This file implements the AbstractLogView base class.
// Most of the actual drawing and event management common to the two views
// is implemented in this class.  The class only calls protected virtual
// functions when view specific behaviour is desired, using the template
// pattern.

#include <iostream>
#include <cassert>

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
#include <QtCore>
#include <QGestureEvent>

#include "log.h"

#include "persistentinfo.h"
#include "highlighterset.h"
#include "logmainview.h"
#include "quickfind.h"
#include "quickfindpattern.h"
#include "overview.h"
#include "configuration.h"

namespace {
int mapPullToFollowLength( int length );
};

namespace {

int countDigits( quint64 n )
{
    if (n == 0)
        return 1;

    // We must force the compiler to not store intermediate results
    // in registers because this causes incorrect result on some
    // systems under optimizations level >0. For the skeptical:
    //
    // #include <math.h>
    // #include <stdlib.h>
    // int main(int argc, char **argv) {
    //     (void)argc;
    //     long long int n = atoll(argv[1]);
    //     return floor( log( n ) / log( 10 ) + 1 );
    // }
    //
    // This is on Thinkpad T60 (Genuine Intel(R) CPU T2300).
    // $ g++ --version
    // g++ (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3
    // $ g++ -O0 -Wall -W -o math math.cpp -lm; ./math 10; echo $?
    // 2
    // $ g++ -O1 -Wall -W -o math math.cpp -lm; ./math 10; echo $?
    // 1
    //
    // A fix is to (1) explicitly place intermediate results in
    // variables *and* (2) [A] mark them as 'volatile', or [B] pass
    // -ffloat-store to g++ (note that approach [A] is more portable).

    volatile qreal ln_n  = qLn( n );
    volatile qreal ln_10 = qLn( 10 );
    volatile qreal lg_n = ln_n / ln_10;
    volatile qreal lg_n_1 = lg_n + 1;
    volatile qreal fl_lg_n_1 = qFloor( lg_n_1 );

    return fl_lg_n_1;
}

} // anon namespace


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
        int initialXPos, int initialYPos,
        int line_width, const QString& line,
        int leftExtraBackgroundPx )
{
    QFontMetrics fm = painter.fontMetrics();
    const int fontHeight = fm.height();
    const int fontAscent = fm.ascent();
    // For some reason on Qt 4.8.2 for Win, maxWidth() is wrong but the
    // following give the right result, not sure why:
    const int fontWidth = fm.width( QChar('a') );

    int xPos = initialXPos;
    int yPos = initialYPos;

    foreach ( Chunk chunk, list ) {
        // Draw each chunk
        // LOG(logDEBUG) << "Chunk: " << chunk.start() << " " << chunk.length();
        QString cutline = line.mid( chunk.start(), chunk.length() );
        const int chunk_width = cutline.length() * fontWidth;
        if ( xPos == initialXPos ) {
            // First chunk, we extend the left background a bit,
            // it looks prettier.
            painter.fillRect( xPos - leftExtraBackgroundPx, yPos,
                    chunk_width + leftExtraBackgroundPx,
                    fontHeight, chunk.backColor() );
        }
        else {
            // other chunks...
            painter.fillRect( xPos, yPos, chunk_width,
                    fontHeight, chunk.backColor() );
        }
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

AbstractLogView::AbstractLogView(const AbstractLogData* newLogData,
        const QuickFindPattern* const quickFindPattern, QWidget* parent) :
    QAbstractScrollArea( parent ),
    followElasticHook_( HOOK_THRESHOLD ),
    lineNumbersVisible_( false ),
    selectionStartPos_(),
    selectionCurrentEndPos_(),
    autoScrollTimer_(),
    selection_(),
    quickFindPattern_( quickFindPattern ),
    quickFind_( newLogData, &selection_, quickFindPattern )
{
    logData = newLogData;

    followMode_ = false;

    selectionStarted_ = false;
    markingClickInitiated_ = false;

    firstLine = 0;
    lastLineAligned = false;
    firstCol = 0;

    overview_ = NULL;
    overviewWidget_ = NULL;

    // Display
    leftMarginPx_ = 0;

    // Fonts (sensible default for overview widget)
    charWidth_ = 1;
    charHeight_ = 10;

    // Create the viewport QWidget
    setViewport( 0 );

    // Hovering
    setMouseTracking( true );
    lastHoveredLine_ = -1;

    // Init the popup menu
    createMenu();

    // Signals
    connect( quickFindPattern_, SIGNAL( patternUpdated() ),
            this, SLOT ( handlePatternUpdated() ) );
    connect( &quickFind_, SIGNAL( notify( const QFNotification& ) ),
            this, SIGNAL( notifyQuickFind( const QFNotification& ) ) );
    connect( &quickFind_, SIGNAL( clearNotification() ),
            this, SIGNAL( clearQuickFindNotification() ) );
    connect( &followElasticHook_, SIGNAL( lengthChanged() ),
            this, SLOT( repaint() ) );
    connect( &followElasticHook_, SIGNAL( hooked( bool ) ),
            this, SIGNAL( followModeChanged( bool ) ) );
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
    static std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );

    if ( mouseEvent->button() == Qt::LeftButton )
    {
        int line = convertCoordToLine( mouseEvent->y() );

        if ( mouseEvent->modifiers() & Qt::ShiftModifier )
        {
            selection_.selectRangeFromPrevious( line );
            emit updateLineNumber( line );
            update();
        }
        else
        {
            if ( mouseEvent->x() < bulletZoneWidthPx_ ) {
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

        // Invalidate our cache
        textAreaCache_.invalid_ = true;
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
        if ( config->mainRegexpType() != ExtendedRegexp )
            addToSearchAction_->setEnabled( false );

        // Display the popup (blocking)
        popupMenu_->exec( QCursor::pos() );
    }

    emit activity();
}

void AbstractLogView::mouseMoveEvent( QMouseEvent* mouseEvent )
{
    // Selection implementation
    if ( selectionStarted_ )
    {
        // Invalidate our cache
        textAreaCache_.invalid_ = true;

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
    else {
        considerMouseHovering( mouseEvent->x(), mouseEvent->y() );
    }
}

void AbstractLogView::mouseReleaseEvent( QMouseEvent* mouseEvent )
{
    if ( markingClickInitiated_ ) {
        markingClickInitiated_ = false;
        int line = convertCoordToLine( mouseEvent->y() );
        if ( line == markingClickLine_ ) {
            // Invalidate our cache
            textAreaCache_.invalid_ = true;

            emit markLine( line );
        }
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
        // Invalidate our cache
        textAreaCache_.invalid_ = true;

        const QPoint pos = convertCoordToFilePos( mouseEvent->pos() );
        selectWordAtPosition( pos );
    }

    emit activity();
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

    bool controlModifier = (keyEvent->modifiers() & Qt::ControlModifier) == Qt::ControlModifier;
    bool shiftModifier = (keyEvent->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier;
    bool noModifier = keyEvent->modifiers() == Qt::NoModifier;

    if ( keyEvent->key() == Qt::Key_Left && noModifier )
        horizontalScrollBar()->triggerAction(QScrollBar::SliderPageStepSub);
    else if ( keyEvent->key() == Qt::Key_Right  && noModifier )
        horizontalScrollBar()->triggerAction(QScrollBar::SliderPageStepAdd);
    else if ( keyEvent->key() == Qt::Key_Home && !controlModifier)
        jumpToStartOfLine();
    else if ( keyEvent->key() == Qt::Key_End  && !controlModifier)
        jumpToRightOfScreen();
    else if ( (keyEvent->key() == Qt::Key_PageDown && controlModifier)
           || (keyEvent->key() == Qt::Key_End && controlModifier) )
    {
        disableFollow(); // duplicate of 'G' action.
        selection_.selectLine( logData->getNbLine() - 1 );
        emit updateLineNumber( logData->getNbLine() - 1 );
        jumpToBottom();
    }
    else if ( (keyEvent->key() == Qt::Key_PageUp && controlModifier)
           || (keyEvent->key() == Qt::Key_Home && controlModifier) )
        selectAndDisplayLine( 0 );
    else if ( keyEvent->key() == Qt::Key_F3 && !shiftModifier )
        searchNext(); // duplicate of 'n' action.
    else if ( keyEvent->key() == Qt::Key_F3 && shiftModifier )
        searchPrevious(); // duplicate of 'N' action.
    else if ( keyEvent->key() == Qt::Key_Space && noModifier )
        emit exitView();
    else {
        const char character = (keyEvent->text())[0].toLatin1();

        if ( keyEvent->modifiers() == Qt::NoModifier &&
                ( character >= '0' ) && ( character <= '9' ) ) {
            // Adds the digit to the timed buffer
            digitsBuffer_.add( character );
        }
        else {
            switch ( (keyEvent->text())[0].toLatin1() ) {
                case 'j':
                    {
                        int delta = qMax( 1, digitsBuffer_.content() );
                        disableFollow();
                        //verticalScrollBar()->triggerAction(
                        //QScrollBar::SliderSingleStepAdd);
                        moveSelection( delta );
                        break;
                    }
                case 'k':
                    {
                        int delta = qMin( -1, - digitsBuffer_.content() );
                        disableFollow();
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
                        selectAndDisplayLine( newLine );
                        break;
                    }
                case 'G':
                    disableFollow();
                    selection_.selectLine( logData->getNbLine() - 1 );
                    emit updateLineNumber( logData->getNbLine() - 1 );
                    emit newSelection( logData->getNbLine() - 1 );
                    jumpToBottom();
                    break;
                case 'n':
                    emit searchNext();
                    break;
                case 'N':
                    emit searchPrevious();
                    break;
                case '*':
                    // Use the selected 'word' and search forward
                    findNextSelected();
                    break;
                case '#':
                    // Use the selected 'word' and search backward
                    findPreviousSelected();
                    break;
                case 'm':
                    {
                        qint64 line = selection_.selectedLine();
                        if ( line >= 0 )
                            emit markLine( line );
                        break;
                    }
                default:
                    keyEvent->ignore();
            }
        }
    }

    if ( keyEvent->isAccepted() ) {
        emit activity();
    }
    else {
        // Only pass bare keys to the superclass this is so that
        // shortcuts such as Ctrl+Alt+Arrow are handled by the parent.
        LOG(logDEBUG) << std::hex << keyEvent->modifiers();
        if ( keyEvent->modifiers() == Qt::NoModifier ||
                keyEvent->modifiers() == Qt::KeypadModifier ) {
            QAbstractScrollArea::keyPressEvent( keyEvent );
        }
    }
}

void AbstractLogView::wheelEvent( QWheelEvent* wheelEvent )
{
    emit activity();

    // LOG(logDEBUG) << "wheelEvent";

    // This is to handle the case where follow mode is on, but the user
    // has moved using the scroll bar. We take them back to the bottom.
    if ( followMode_ )
        jumpToBottom();

    int y_delta = 0;
    if ( verticalScrollBar()->value() == verticalScrollBar()->maximum() ) {
        // First see if we need to block the elastic (on Mac)
        if ( wheelEvent->phase() == Qt::ScrollBegin )
            followElasticHook_.hold();
        else if ( wheelEvent->phase() == Qt::ScrollEnd )
            followElasticHook_.release();

        auto pixel_delta = wheelEvent->pixelDelta();

        if ( pixel_delta.isNull() ) {
            y_delta = wheelEvent->angleDelta().y() / 0.7;
        }
        else {
            y_delta = pixel_delta.y();
        }

        // LOG(logDEBUG) << "Elastic " << y_delta;
        followElasticHook_.move( - y_delta );
    }

    // LOG(logDEBUG) << "Length = " << followElasticHook_.length();
    if ( followElasticHook_.length() == 0 && !followElasticHook_.isHooked() ) {
        QAbstractScrollArea::wheelEvent( wheelEvent );
    }
}

void AbstractLogView::resizeEvent( QResizeEvent* )
{
    if ( logData == NULL )
        return;

    LOG(logDEBUG) << "resizeEvent received";

    updateDisplaySize();
}

bool AbstractLogView::event( QEvent* e )
{
    LOG(logDEBUG4) << "Event! Type: " << e->type();

    // Make sure we ignore the gesture events as
    // they seem to be accepted by default.
    if ( e->type() == QEvent::Gesture ) {
        auto gesture_event = dynamic_cast<QGestureEvent*>( e );
        if ( gesture_event ) {
            foreach( QGesture* gesture, gesture_event->gestures() ) {
                LOG(logDEBUG4) << "Gesture: " << gesture->gestureType();
                gesture_event->ignore( gesture );
            }

            // Ensure the event is sent up to parents who might care
            return false;
        }
    }

    return QAbstractScrollArea::event( e );
}

void AbstractLogView::scrollContentsBy( int dx, int dy )
{
    LOG(logDEBUG) << "scrollContentsBy received " << dy
        << "position " << verticalScrollBar()->value();

    int32_t last_top_line = ( logData->getNbLine() - getNbVisibleLines() );
    if ( ( last_top_line > 0 ) && verticalScrollBar()->value() > last_top_line ) {
        // The user is going further than the last line, we need to lock the last line at the bottom
        LOG(logDEBUG) << "scrollContentsBy beyond!";
        firstLine = last_top_line;
        lastLineAligned = true;
    }
    else {
        firstLine = verticalScrollBar()->value();
        lastLineAligned = false;
    }

    firstCol  = (firstCol - dx) > 0 ? firstCol - dx : 0;
    LineNumber last_line  = firstLine + getNbVisibleLines();

    // Update the overview if we have one
    if ( overview_ != NULL )
        overview_->updateCurrentPosition( firstLine, last_line );

    // Are we hovering over a new line?
    const QPoint mouse_pos = mapFromGlobal( QCursor::pos() );
    considerMouseHovering( mouse_pos.x(), mouse_pos.y() );

    // Redraw
    update();
}

void AbstractLogView::paintEvent( QPaintEvent* paintEvent )
{
    const QRect invalidRect = paintEvent->rect();
    if ( (invalidRect.isEmpty()) || (logData == NULL) )
        return;

    LOG(logDEBUG4) << "paintEvent received, firstLine=" << firstLine
        << " lastLineAligned=" << lastLineAligned
        << " rect: " << invalidRect.topLeft().x() <<
        ", " << invalidRect.topLeft().y() <<
        ", " << invalidRect.bottomRight().x() <<
        ", " << invalidRect.bottomRight().y();

#ifdef GLOGG_PERF_MEASURE_FPS
    static uint32_t maxline = logData->getNbLine();
    if ( ! perfCounter_.addEvent() && logData->getNbLine() > maxline ) {
        LOG(logWARNING) << "Redraw per second: " << perfCounter_.readAndReset()
            << " lines: " << logData->getNbLine();
        perfCounter_.addEvent();
        maxline = logData->getNbLine();
    }
#endif

    auto start = std::chrono::system_clock::now();

    // Can we use our cache?
    int32_t delta_y = textAreaCache_.first_line_ - firstLine;

    if ( textAreaCache_.invalid_ || ( textAreaCache_.first_column_ != firstCol ) ) {
        // Force a full redraw
        delta_y = INT32_MAX;
    }

    if ( delta_y != 0 ) {
        // Full or partial redraw
        drawTextArea( &textAreaCache_.pixmap_, delta_y );

        textAreaCache_.invalid_      = false;
        textAreaCache_.first_line_   = firstLine;
        textAreaCache_.first_column_ = firstCol;

        LOG(logDEBUG) << "End of writing " <<
            std::chrono::duration_cast<std::chrono::microseconds>
            ( std::chrono::system_clock::now() - start ).count();
    }
    else {
        // Use the cache as is: nothing to do!
    }

    // Height including the potentially invisible last line
    const int whole_height = getNbVisibleLines() * charHeight_;
    // Height in pixels of the "pull to follow" bottom bar.
    int pullToFollowHeight = mapPullToFollowLength( followElasticHook_.length() )
        + ( followElasticHook_.isHooked() ?
                ( whole_height - viewport()->height() ) + PULL_TO_FOLLOW_HOOKED_HEIGHT : 0 );

    if ( pullToFollowHeight
            && ( pullToFollowCache_.nb_columns_ != getNbVisibleCols() ) ) {
        LOG(logDEBUG) << "Drawing pull to follow bar";
        pullToFollowCache_.pixmap_ = drawPullToFollowBar(
                viewport()->width(), viewport()->devicePixelRatio() );
        pullToFollowCache_.nb_columns_ = getNbVisibleCols();
    }

    QPainter devicePainter( viewport() );
    int drawingTopPosition = - pullToFollowHeight;
    int drawingPullToFollowTopPosition = drawingTopPosition + whole_height;

    // This is to cover the special case where there is less than a screenful
    // worth of data, we want to see the document from the top, rather than
    // pushing the first couple of lines above the viewport.
    if ( followElasticHook_.isHooked() && ( logData->getNbLine() < getNbVisibleLines() ) ) {
        drawingTopOffset_ = 0;
        drawingTopPosition += ( whole_height - viewport()->height() ) + PULL_TO_FOLLOW_HOOKED_HEIGHT;
        drawingPullToFollowTopPosition = drawingTopPosition + viewport()->height() - PULL_TO_FOLLOW_HOOKED_HEIGHT;
    }
    // This is the case where the user is on the 'extra' slot at the end
    // and is aligned on the last line (but no elastic shown)
    else if ( lastLineAligned && !followElasticHook_.isHooked() ) {
        drawingTopOffset_ = - ( whole_height - viewport()->height() );
        drawingTopPosition += drawingTopOffset_;
        drawingPullToFollowTopPosition = drawingTopPosition + whole_height;
    }
    else {
        drawingTopOffset_ = - pullToFollowHeight;
    }

    devicePainter.drawPixmap( 0, drawingTopPosition, textAreaCache_.pixmap_ );

    // Draw the "pull to follow" zone if needed
    if ( pullToFollowHeight ) {
        devicePainter.drawPixmap( 0,
                drawingPullToFollowTopPosition,
                pullToFollowCache_.pixmap_ );
    }

    LOG(logDEBUG) << "End of repaint " <<
        std::chrono::duration_cast<std::chrono::microseconds>
        ( std::chrono::system_clock::now() - start ).count();
}

// These two functions are virtual and this implementation is clearly
// only valid for a non-filtered display.
// We count on the 'filtered' derived classes to override them.
qint64 AbstractLogView::displayLineNumber( int lineNumber ) const
{
    return lineNumber + 1; // show a 1-based index
}

qint64 AbstractLogView::maxDisplayLineNumber() const
{
    return logData->getNbLine();
}

void AbstractLogView::setOverview( Overview* overview,
       OverviewWidget* overview_widget )
{
    overview_ = overview;
    overviewWidget_ = overview_widget;

    if ( overviewWidget_ ) {
        connect( overviewWidget_, SIGNAL( lineClicked ( int ) ),
                this, SLOT( jumpToLine( int ) ) );
    }
    refreshOverview();
}

LineNumber AbstractLogView::getViewPosition() const
{
    LineNumber line;

    qint64 m_line = selection_.selectedLine();
    if ( m_line >= 0 ) {
        line = m_line;
    }
    else {
        // Middle of the view
        line = firstLine + getNbVisibleLines() / 2;
    }

    return line;
}

void AbstractLogView::searchUsingFunction(
        qint64 (QuickFind::*search_function)() )
{
    disableFollow();

    int line = (quickFind_.*search_function)();
    if ( line >= 0 ) {
        LOG(logDEBUG) << "Found line " << line;
        displayLine( line );
        emit updateLineNumber( line );
    }
}

void AbstractLogView::searchForward()
{
    searchUsingFunction( &QuickFind::searchForward );
}

void AbstractLogView::searchBackward()
{
    searchUsingFunction( &QuickFind::searchBackward );
}

void AbstractLogView::incrementallySearchForward()
{
    searchUsingFunction( &QuickFind::incrementallySearchForward );
}

void AbstractLogView::incrementallySearchBackward()
{
    searchUsingFunction( &QuickFind::incrementallySearchBackward );
}

void AbstractLogView::incrementalSearchAbort()
{
    quickFind_.incrementalSearchAbort();
    emit changeQuickFind(
            "",
            QuickFindMux::Forward );
}

void AbstractLogView::incrementalSearchStop()
{
    quickFind_.incrementalSearchStop();
}

void AbstractLogView::followSet( bool checked )
{
    followMode_ = checked;
    followElasticHook_.hook( checked );
    update();
    if ( checked )
        jumpToBottom();
}

void AbstractLogView::refreshOverview()
{
    assert( overviewWidget_ );

    // Create space for the Overview if needed
    if ( ( getOverview() != NULL ) && getOverview()->isVisible() ) {
        setViewportMargins( 0, 0, OVERVIEW_WIDTH, 0 );
        overviewWidget_->show();
    }
    else {
        setViewportMargins( 0, 0, 0, 0 );
        overviewWidget_->hide();
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
                selection_.getSelectedText( logData ),
                QuickFindMux::Forward );
        emit searchNext();
    }
}

// Find next previous of the selected text (#)
void AbstractLogView::findPreviousSelected()
{
    if ( selection_.isPortion() ) {
        emit changeQuickFind(
                selection_.getSelectedText( logData ),
                QuickFindMux::Backward );
        emit searchNext();
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
    updateScrollBars();

    // Calculate the index of the last line shown
    LineNumber last_line = std::min( static_cast<int64_t>( logData->getNbLine() ),
            static_cast<int64_t>( firstLine + getNbVisibleLines() ) );

    // Reset the QuickFind in case we have new stuff to search into
    quickFind_.resetLimits();

    if ( followMode_ )
        jumpToBottom();

    // Update the overview if we have one
    if ( overview_ != NULL )
        overview_->updateCurrentPosition( firstLine, last_line );

    // Invalidate our cache
    textAreaCache_.invalid_ = true;

    // Repaint!
    update();
}

void AbstractLogView::updateDisplaySize()
{
    // Font is assumed to be mono-space (is restricted by options dialog)
    QFontMetrics fm = fontMetrics();
    charHeight_ = fm.height();
    // For some reason on Qt 4.8.2 for Win, maxWidth() is wrong but the
    // following give the right result, not sure why:
    charWidth_ = fm.width( QChar('a') );

    // Update the scroll bars
    updateScrollBars();
    verticalScrollBar()->setPageStep( getNbVisibleLines() );

    if ( followMode_ )
        jumpToBottom();

    LOG(logDEBUG) << "viewport.width()=" << viewport()->width();
    LOG(logDEBUG) << "viewport.height()=" << viewport()->height();
    LOG(logDEBUG) << "width()=" << width();
    LOG(logDEBUG) << "height()=" << height();

    if ( overviewWidget_ )
        overviewWidget_->setGeometry( viewport()->width() + 2, 1,
                OVERVIEW_WIDTH - 1, viewport()->height() );

    // Our text area cache is now invalid
    textAreaCache_.invalid_ = true;
    textAreaCache_.pixmap_  = QPixmap {
        viewport()->width() * viewport()->devicePixelRatio(),
        static_cast<int32_t>( getNbVisibleLines() ) * charHeight_ * viewport()->devicePixelRatio() };
    textAreaCache_.pixmap_.setDevicePixelRatio( viewport()->devicePixelRatio() );
}

int AbstractLogView::getTopLine() const
{
    return firstLine;
}

QString AbstractLogView::getSelection() const
{
    return selection_.getSelectedText( logData );
}

void AbstractLogView::selectAll()
{
    selection_.selectRange( 0, logData->getNbLine() - 1 );
    textAreaCache_.invalid_ = true;
    update();
}

void AbstractLogView::selectAndDisplayLine( int line )
{
    disableFollow();
    selection_.selectLine( line );
    displayLine( line );
    emit updateLineNumber( line );
    emit newSelection( line );
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

void AbstractLogView::setLineNumbersVisible( bool lineNumbersVisible )
{
    lineNumbersVisible_ = lineNumbersVisible;
}

void AbstractLogView::forceRefresh()
{
    // Invalidate our cache
    textAreaCache_.invalid_ = true;
}

//
// Private functions
//

// Returns the number of lines visible in the viewport
LineNumber AbstractLogView::getNbVisibleLines() const
{
    return static_cast<LineNumber>( viewport()->height() / charHeight_ + 1 );
}

// Returns the number of columns visible in the viewport
int AbstractLogView::getNbVisibleCols() const
{
    return ( viewport()->width() - leftMarginPx_ ) / charWidth_ + 1;
}

// Converts the mouse x, y coordinates to the line number in the file
int AbstractLogView::convertCoordToLine(int yPos) const
{
    int line = firstLine + ( yPos - drawingTopOffset_ ) / charHeight_;

    return line;
}

// Converts the mouse x, y coordinates to the char coordinates (in the file)
// This function ensure the pos exists in the file.
QPoint AbstractLogView::convertCoordToFilePos( const QPoint& pos ) const
{
    int line = convertCoordToLine( pos.y() );
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

    LOG(logDEBUG4) << "AbstractLogView::convertCoordToFilePos col="
        << column << " line=" << line;
    QPoint point( column, line );

    return point;
}

// Makes the widget adjust itself to display the passed line.
// Doing so, it will throw itself a scrollContents event.
void AbstractLogView::displayLine( LineNumber line )
{
    // If the line is already the screen
    if ( ( line >= firstLine ) &&
         ( line < ( firstLine + getNbVisibleLines() ) ) ) {
        // Invalidate our cache
        textAreaCache_.invalid_ = true;

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
    emit newSelection( new_line );
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
    for ( auto i = firstLine; i <= ( firstLine + getNbVisibleLines() ); i++ ) {
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

void AbstractLogView::considerMouseHovering( int x_pos, int y_pos )
{
    int line = convertCoordToLine( y_pos );
    if ( ( x_pos < leftMarginPx_ )
            && ( line >= 0 )
            && ( line < logData->getNbLine() ) ) {
        // Mouse moved in the margin, send event up
        // (possibly to highlight the overview)
        if ( line != lastHoveredLine_ ) {
            LOG(logDEBUG) << "Mouse moved in margin line: " << line;
            emit mouseHoveredOverLine( line );
            lastHoveredLine_ = line;
        }
    }
    else {
        if ( lastHoveredLine_ != -1 ) {
            emit mouseLeftHoveringZone();
            lastHoveredLine_ = -1;
        }
    }
}

void AbstractLogView::updateScrollBars()
{
    verticalScrollBar()->setRange( 0, std::max( 0LL,
            logData->getNbLine() - getNbVisibleLines() + 1 ) );

    const int hScrollMaxValue = std::max( 0,
            logData->getMaxLength() - getNbVisibleCols() + 1 );
    horizontalScrollBar()->setRange( 0, hScrollMaxValue );
}

void AbstractLogView::drawTextArea( QPaintDevice* paint_device, int32_t )
{
    // LOG( logDEBUG ) << "devicePixelRatio: " << viewport()->devicePixelRatio();
    // LOG( logDEBUG ) << "viewport size: " << viewport()->size().width();
    // LOG( logDEBUG ) << "pixmap size: " << textPixmap.width();
    // Repaint the viewport
    QPainter painter( paint_device );
    // LOG( logDEBUG ) << "font: " << viewport()->font().family().toStdString();
    // LOG( logDEBUG ) << "font painter: " << painter.font().family().toStdString();

    painter.setFont( this->font() );

    const int fontHeight = charHeight_;
    const int fontAscent = painter.fontMetrics().ascent();
    const int nbCols = getNbVisibleCols();
    const int paintDeviceHeight = paint_device->height() / viewport()->devicePixelRatio();
    const int paintDeviceWidth = paint_device->width() / viewport()->devicePixelRatio();
    const QPalette& palette = viewport()->palette();
    std::shared_ptr<const HighlighterSet> highlighterSet =
        Persistent<HighlighterSet>( "highlighterSet" );
    QColor foreColor, backColor;

    static const QBrush normalBulletBrush = QBrush( Qt::white );
    static const QBrush matchBulletBrush = QBrush( Qt::red );
    static const QBrush markBrush = QBrush( "dodgerblue" );

    static const int SEPARATOR_WIDTH = 1;
    static const qreal BULLET_AREA_WIDTH = 11;
    static const int CONTENT_MARGIN_WIDTH = 1;
    static const int LINE_NUMBER_PADDING = 3;

    // First check the lines to be drawn are within range (might not be the case if
    // the file has just changed)
    const int64_t lines_in_file = logData->getNbLine();

    if ( firstLine > lines_in_file )
        firstLine = lines_in_file ? lines_in_file - 1 : 0;

    const int64_t nbLines = std::min(
            static_cast<int64_t>( getNbVisibleLines() ), lines_in_file - firstLine );

    const int bottomOfTextPx = nbLines * fontHeight;

    LOG(logDEBUG) << "drawing lines from " << firstLine << " (" << nbLines << " lines)";
    LOG(logDEBUG) << "bottomOfTextPx: " << bottomOfTextPx;
    LOG(logDEBUG) << "Height: " << paintDeviceHeight;

    // Lines to write
    const QStringList lines = logData->getExpandedLines( firstLine, nbLines );

    // First draw the bullet left margin
    painter.setPen(palette.color(QPalette::Text));
    painter.fillRect( 0, 0,
                      BULLET_AREA_WIDTH, paintDeviceHeight,
                      Qt::darkGray );

    // Column at which the content should start (pixels)
    qreal contentStartPosX = BULLET_AREA_WIDTH + SEPARATOR_WIDTH;

    // This is also the bullet zone width, used for marking clicks
    bulletZoneWidthPx_ = contentStartPosX;

    // Update the length of line numbers
    const int nbDigitsInLineNumber = countDigits( maxDisplayLineNumber() );

    // Draw the line numbers area
    int lineNumberAreaStartX = 0;
    if ( lineNumbersVisible_ ) {
        int lineNumberWidth = charWidth_ * nbDigitsInLineNumber;
        int lineNumberAreaWidth =
            2 * LINE_NUMBER_PADDING + lineNumberWidth;
        lineNumberAreaStartX = contentStartPosX;

        painter.setPen(palette.color(QPalette::Text));
        /* Not sure if it looks good...
        painter.drawLine( contentStartPosX + lineNumberAreaWidth,
                          0,
                          contentStartPosX + lineNumberAreaWidth,
                          viewport()->height() );
        */
        painter.fillRect( contentStartPosX - SEPARATOR_WIDTH, 0,
                          lineNumberAreaWidth + SEPARATOR_WIDTH, paintDeviceHeight,
                          Qt::lightGray );

        // Update for drawing the actual text
        contentStartPosX += lineNumberAreaWidth;
    }
    else {
        painter.fillRect( contentStartPosX - SEPARATOR_WIDTH, 0,
                          SEPARATOR_WIDTH + 1, paintDeviceHeight,
                          Qt::lightGray );
        // contentStartPosX += SEPARATOR_WIDTH;
    }

    painter.drawLine( BULLET_AREA_WIDTH, 0,
                      BULLET_AREA_WIDTH, paintDeviceHeight - 1 );

    // This is the total width of the 'margin' (including line number if any)
    // used for mouse calculation etc...
    leftMarginPx_ = contentStartPosX + SEPARATOR_WIDTH;

    // Then draw each line
    for (int i = 0; i < nbLines; i++) {
        const LineNumber line_index = i + firstLine;

        // Position in pixel of the base line of the line to print
        const int yPos = i * fontHeight;
        const int xPos = contentStartPosX + CONTENT_MARGIN_WIDTH;

        // string to print, cut to fit the length and position of the view
        const QString line = lines[i];
        const QString cutLine = line.mid( firstCol, nbCols );

        if ( selection_.isLineSelected( line_index ) ) {
            // Reverse the selected line
            foreColor = palette.color( QPalette::HighlightedText );
            backColor = palette.color( QPalette::Highlight );
            painter.setPen(palette.color(QPalette::Text));
        }
        else if ( highlighterSet->matchLine( logData->getLineString( line_index ),
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
            selection_.getPortionForLine( line_index, &sel_start, &sel_end );
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
                    viewport()->width(), cutLine,
                    CONTENT_MARGIN_WIDTH );
        }
        else {
            // Nothing to be highlighted, we print the whole line!
            painter.fillRect( xPos - CONTENT_MARGIN_WIDTH, yPos,
                    viewport()->width(), fontHeight, backColor );
            // (the rectangle is extended on the left to cover the small
            // margin, it looks better (LineDrawer does the same) )
            painter.setPen( foreColor );
            painter.drawText( xPos, yPos + fontAscent, cutLine );
        }

        // Then draw the bullet
        painter.setPen( palette.color( QPalette::Text ) );
        const qreal circleSize = 3;
        const qreal arrowHeight = 4;
        const qreal middleXLine = BULLET_AREA_WIDTH / 2;
        const qreal middleYLine = yPos + (fontHeight / 2);

        const LineType line_type = lineType( line_index );
        if ( line_type == Marked ) {
            // A pretty arrow if the line is marked
            const QPointF points[7] = {
                QPointF(1, middleYLine - 2),
                QPointF(middleXLine, middleYLine - 2),
                QPointF(middleXLine, middleYLine - arrowHeight),
                QPointF(BULLET_AREA_WIDTH - 1, middleYLine),
                QPointF(middleXLine, middleYLine + arrowHeight),
                QPointF(middleXLine, middleYLine + 2),
                QPointF(1, middleYLine + 2 ),
            };

            painter.setBrush( markBrush );
            painter.drawPolygon( points, 7 );
        }
        else {
            // For pretty circles
            painter.setRenderHint( QPainter::Antialiasing );

            if ( lineType( line_index ) == Match )
                painter.setBrush( matchBulletBrush );
            else
                painter.setBrush( normalBulletBrush );
            painter.drawEllipse( middleXLine - circleSize,
                    middleYLine - circleSize,
                    circleSize * 2, circleSize * 2 );
        }

        // Draw the line number
        if ( lineNumbersVisible_ ) {
            static const QString lineNumberFormat( "%1" );
            const QString& lineNumberStr =
                lineNumberFormat.arg( displayLineNumber( line_index ),
                        nbDigitsInLineNumber );
            painter.setPen( palette.color( QPalette::Text ) );
            painter.drawText( lineNumberAreaStartX + LINE_NUMBER_PADDING,
                    yPos + fontAscent, lineNumberStr );
        }
    } // For each line

    if ( bottomOfTextPx < paintDeviceHeight ) {
        // The lines don't cover the whole device
        painter.fillRect( contentStartPosX, bottomOfTextPx,
                paintDeviceWidth - contentStartPosX,
                paintDeviceHeight, palette.color( QPalette::Window ) );
    }
}

// Draw the "pull to follow" bar and return a pixmap.
// The width is passed in "logic" pixels.
QPixmap AbstractLogView::drawPullToFollowBar( int width, float pixel_ratio )
{
    static constexpr int barWidth = 40;
    QPixmap pixmap ( static_cast<float>( width ) * pixel_ratio, barWidth * 6.0 );
    pixmap.setDevicePixelRatio( pixel_ratio );
    pixmap.fill( this->palette().color( this->backgroundRole() ) );
    const int nbBars = width / (barWidth * 2) + 1;

    QPainter painter( &pixmap );
    painter.setPen( QPen( QColor( 0, 0, 0, 0 ) ) );
    painter.setBrush( QBrush( QColor( "lightyellow" ) ) );

    for ( int i = 0; i < nbBars; ++i ) {
        QPoint points[4] = {
            { (i*2+1)*barWidth, 0 },
            { 0, (i*2+1)*barWidth },
            { 0, (i+1)*2*barWidth },
            { (i+1)*2*barWidth, 0 }
        };
        painter.drawConvexPolygon( points, 4 );
    }

    return pixmap;
}

void AbstractLogView::disableFollow()
{
    emit followModeChanged( false );
    followElasticHook_.hook( false );
}

namespace {

// Convert the length of the pull to follow bar to pixels
int mapPullToFollowLength( int length )
{
    return length / 14;
}

};

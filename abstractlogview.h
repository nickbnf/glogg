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

#ifndef ABSTRACTLOGVIEW_H
#define ABSTRACTLOGVIEW_H

#include <QAbstractScrollArea>
#include <QBasicTimer>

#include "abstractlogdata.h"
#include "selection.h"
#include "quickfind.h"
#include "overviewwidget.h"

class QMenu;
class QAction;

class LineChunk
{
  public:
    enum ChunkType {
        Normal,
        Highlighted,
        Selected,
    };

    LineChunk( int first_col, int end_col, ChunkType type );

    int start() const { return start_; }
    int end() const { return end_; }
    ChunkType type() const { return type_; }

    // Returns 'true' if the selection is part of this chunk
    // (at least partially), if so, it should be replaced by the list returned
    QList<LineChunk> select( int selection_start, int selection_end ) const;

  private:
    int start_;
    int end_;
    ChunkType type_;
};

// Utility class for syntax colouring.
// It stores the chunks of line to draw
// each chunk having a different colour
class LineDrawer
{
  public:
    LineDrawer( const QColor& back_color) :
        list(), backColor_( back_color ) { };

    // Add a chunk of line using the given colours.
    // Both first_col and last_col are included
    // An empty chunk will be ignored.
    // the first column will be set to 0 if negative
    // The column are relative to the screen
    void addChunk( int first_col, int last_col, QColor fore, QColor back );
    void addChunk( const LineChunk& chunk, QColor fore, QColor back );

    // Draw the current line of text using the given painter,
    // in the passed block (in pixels)
    // The line must be cut to fit on the screen.
    void draw( QPainter& painter, int xPos, int yPos,
               int line_width, const QString& line );

  private:
    class Chunk {
      public:
        // Create a new chunk
        Chunk( int start, int length, QColor fore, QColor back )
            : start_( start ), length_( length ),
            foreColor_ ( fore ), backColor_ ( back ) { };

        // Accessors
        int start() const { return start_; }
        int length() const { return length_; }
        const QColor& foreColor() const { return foreColor_; }
        const QColor& backColor() const { return backColor_; }

      private:
        int start_;
        int length_;
        QColor foreColor_;
        QColor backColor_;
    };
    QList<Chunk> list;
    QColor backColor_;
};


// Utility class representing a buffer for number entered on the keyboard
// The buffer keep at most 7 digits, and reset itself after a timeout.
class DigitsBuffer : public QObject
{
  Q_OBJECT

  public:
    DigitsBuffer();

    // Reset the buffer.
    void reset();
    // Add a single digit to the buffer (discarded if it's not a digit),
    // the timeout timer is reset.
    void add( char character );
    // Get the content of the buffer (0 if empty) and reset it.
    int content();

  protected:
    void timerEvent( QTimerEvent* event );

  private:
    // Duration of the timeout in milliseconds.
    static const int timeout_;

    QString digits_;

    QBasicTimer timer_;
};

class Overview;

// Base class representing the log view widget.
// It can be either the top (full) or bottom (filtered) view.
class AbstractLogView : public QAbstractScrollArea
{
  Q_OBJECT

  public:
    // Constructor of the widget, the data set is passed.
    // The caller retains ownership of the data set.
    // The pointer to the QFP is used for colouring and QuickFind searches
    AbstractLogView( const AbstractLogData* newLogData,
            const QuickFindPattern* const quickFind, QWidget* parent=0 );
    ~AbstractLogView();

    // Refresh the widget when the data set has changed.
    void updateData();
    // Instructs the widget to update it's content geometry,
    // used when the font is changed.
    void updateDisplaySize();
    // Return the line number of the top line of the view
    int getTopLine() const;
    // Return the line number of the line selected, or -1 if none.
    QString getSelection() const;

  protected:
    void mousePressEvent( QMouseEvent* mouseEvent );
    void mouseMoveEvent( QMouseEvent* mouseEvent );
    void mouseReleaseEvent( QMouseEvent* );
    void mouseDoubleClickEvent( QMouseEvent* mouseEvent );
    void timerEvent( QTimerEvent* timerEvent );
    void changeEvent( QEvent* changeEvent );
    void paintEvent( QPaintEvent* paintEvent );
    void resizeEvent( QResizeEvent* resizeEvent );
    void scrollContentsBy( int dx, int dy );
    void keyPressEvent( QKeyEvent* keyEvent );
    void wheelEvent( QWheelEvent* wheelEvent );

    // Must be implemented to return wether the line number is
    // a match, a mark or just a normal line (used for coloured bullets)
    enum LineType { Normal, Marked, Match };
    virtual LineType lineType( int lineNumber ) const = 0;

    // Line number to display for line at the given index
    virtual qint64 displayLineNumber( int lineNumber ) const;
    virtual qint64 maxDisplayLineNumber() const;

    // Get the overview associated with this view, or NULL if there is none
    Overview* getOverview() const { return overview_; }
    // Set the Overview
    void setOverview( Overview* overview );

  signals:
    // Sent when a new line has been selected by the user.
    void newSelection(int line);
    // Sent up to the MainWindow to disable the follow mode
    void followDisabled();
    // Sent when the view wants the QuickFind widget pattern to change.
    void changeQuickFind( const QString& newPattern );
    // Sent up when the current line number is updated
    void updateLineNumber( int line );
    // Sent up when quickFind wants to show a message to the user.
    void notifyQuickFind( const QString& message );
    // Sent up when quickFind wants to clear the notification.
    void clearQuickFindNotification();
    // Sent when the view ask for a line to be marked
    // (click in the left margin).
    void markLine( qint64 line );
    // Sent up when the user wants to add the selection to the search
    void addToSearch( const QString& selection );

  public slots:
    // Makes the widget select and display the passed line.
    // Scrolling as necessary
    void selectAndDisplayLine( int line );

    // Use the current QFP to go and select the next match.
    void searchForward();
    // Use the current QFP to go and select the previous match.
    void searchBackward();

    // Signals the follow mode has been enabled.
    void followSet( bool checked );

    // Signal the on/off status of the overview has been changed.
    void refreshOverview();

    // Make the view jump to the specified line, regardless of weither it
    // is on the screen or not.
    // (does NOT emit followDisabled() )
    void jumpToLine( int line );

    // Configure the setting of whether to show line number margin
    void setLineNumbersVisible( bool lineNumbersVisible );

  private slots:
    void handlePatternUpdated();
    void addToSearch();
    void findNextSelected();
    void findPreviousSelected();
    void copy();

  private:
    // Constants
    static const int OVERVIEW_WIDTH;

    // Total size of all margins in pixels
    int leftMarginPx_;

    // Digits buffer
    DigitsBuffer digitsBuffer_;

    // Follow mode
    bool followMode_;

    // Whether to show line numbers or not
    bool lineNumbersVisible_;

    // Pointer to the CrawlerWidget's data set
    const AbstractLogData* logData;

    // Pointer to the Overview object
    Overview* overview_;

    bool selectionStarted_;
    // Start of the selection (characters)
    QPoint selectionStartPos_;
    // Current end of the selection (characters)
    QPoint selectionCurrentEndPos_;
    QBasicTimer autoScrollTimer_;

    // Marks (left margin click)
    bool markingClickInitiated_;
    qint64 markingClickLine_;

    Selection selection_;
    qint64 firstLine;
    qint64 lastLine;
    int firstCol;

    // Text handling
    int charWidth_;
    int charHeight_;

    // Popup menu
    QMenu* popupMenu_;
    QAction* copyAction_;
    QAction* findNextAction_;
    QAction* findPreviousAction_;
    QAction* addToSearchAction_;

    // Pointer to the CrawlerWidget's QFP object
    const QuickFindPattern* const quickFindPattern_;
    // Our own QuickFind object
    QuickFind quickFind_;

    OverviewWidget overviewWidget_;

    int getNbVisibleLines() const;
    int getNbVisibleCols() const;
    void convertCoordToFilePos( const QPoint& pos,
            int* line, int* column ) const;
    QPoint convertCoordToFilePos( const QPoint& pos ) const;
    int convertCoordToLine( int yPos ) const;
    int convertCoordToColumn( int xPos ) const;
    void displayLine( int line );
    void moveSelection( int y );
    void jumpToStartOfLine();
    void jumpToEndOfLine();
    void jumpToRightOfScreen();
    void jumpToTop();
    void jumpToBottom();
    void selectWordAtPosition( const QPoint& pos );

    void createMenu();

    // Search functions (for n/N)
    void searchNext();
    void searchPrevious();

    // Utils functions
    bool isCharWord( char c );
    void updateGlobalSelection();
};

#endif

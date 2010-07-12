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

#ifndef ABSTRACTLOGVIEW_H
#define ABSTRACTLOGVIEW_H

#include <QAbstractScrollArea>
#include <QBasicTimer>

#include "abstractlogdata.h"
#include "selection.h"

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


// Base class representing the log view widget.
// It can be either the top (full) or bottom (filtered) view.
class AbstractLogView : public QAbstractScrollArea
{
  Q_OBJECT

  public:
    // Constructor of the widget, the data set is passed.
    // The caller retains ownership of the data set.
    AbstractLogView(const AbstractLogData* newLogData, QWidget *parent=0);

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
    void timerEvent( QTimerEvent* timerEvent );
    void changeEvent( QEvent* changeEvent );
    void paintEvent( QPaintEvent* paintEvent );
    void resizeEvent( QResizeEvent* resizeEvent );
    void scrollContentsBy( int dx, int dy );
    void keyPressEvent( QKeyEvent* keyEvent );

    // Must be implemented to return wether the line number is
    // a match or not (used for coloured bullets)
    virtual bool isLineMatching( int lineNumber ) = 0;

  signals:
    // Sent when a new line has been selected by the user.
    void newSelection(int line);

  public slots:
    // Makes the widget adjust itself to display the passed line.
    // Doing so, it will throw itself a scrollContents event.
    void displayLine(int line);

  private:
    // Constants
    static const int bulletLineX_;
    static const int leftMarginPx_;

    const AbstractLogData* logData;

    bool selectionStarted_;
    // Start of the selection (characters)
    QPoint selectionStartPos_;
    // Current end of the selection (characters)
    QPoint selectionCurrentEndPos_;
    QBasicTimer autoScrollTimer_;

    Selection selection_;
    qint64 firstLine;
    qint64 lastLine;
    int firstCol;

    // Text handling
    bool useFixedFont_;
    int charWidth_;             // Must only be used if useFixedFont_ == true
    int charHeight_;

    int getNbVisibleLines() const;
    int getNbVisibleCols() const;
    void convertCoordToFilePos( const QPoint& pos,
            int* line, int* column ) const;
    QPoint convertCoordToFilePos( const QPoint& pos ) const;
    int convertCoordToLine( int yPos ) const;
    int convertCoordToColumn( int xPos ) const;
    void moveSelection( int y );
    void jumpToStartOfLine();
    void jumpToEndOfLine();
    void jumpToRightOfScreen();
    void jumpToTop();
    void jumpToBottom();
};

#endif

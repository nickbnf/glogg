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

#include "abstractlogdata.h"
#include "selection.h"

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
    void mousePressEvent(QMouseEvent* mouseEvent);
    void paintEvent(QPaintEvent* paintEvent);
    void resizeEvent(QResizeEvent* resizeEvent);
    void scrollContentsBy(int dx, int dy);
    void keyPressEvent(QKeyEvent* keyEvent);

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
    const AbstractLogData* logData;

    Selection selection_;
    qint64 firstLine;
    qint64 lastLine;
    int firstCol;

    int getNbVisibleLines() const;
    int getNbVisibleCols() const;
    int convertCoordToLine(int yPos) const;
    void moveSelection( int y );
    void jumpToTop();
    void jumpToBottom();
};

#endif

/*
 * Copyright (C) 2011 Nicolas Bonnefon and other contributors
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

#ifndef OVERVIEWWIDGET_H
#define OVERVIEWWIDGET_H

#include <QWidget>

class Overview;

class OverviewWidget : public QWidget
{
  Q_OBJECT

  public:
    OverviewWidget( QWidget* parent );

    // Associate the widget with an Overview object.
    void setOverview( Overview* overview ) { overview_ = overview; }

  protected:
    void paintEvent( QPaintEvent* paintEvent );
    void mousePressEvent( QMouseEvent* mouseEvent );
    void mouseMoveEvent( QMouseEvent* mouseEvent );

  signals:
    // Sent when the user click on a line in the Overview.
    void lineClicked( int line );

  private:
    // Constants
    static const int LINE_MARGIN;

    Overview* overview_;

    void handleMousePress( int position );
};

#endif

/*
 * Copyright (C) 2014 Nicolas Bonnefon and other contributors
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

#ifndef TABBEDCRAWLERWIDGET_H
#define TABBEDCRAWLERWIDGET_H

#include <QTabWidget>
#include <QTabBar>

// This class represents glogg's main widget, a tabbed
// group of CrawlerWidgets.
// This is a very slightly customised QTabWidget, with
// a particular style.
class TabbedCrawlerWidget : public QTabWidget
{
  Q_OBJECT
    public:
      TabbedCrawlerWidget();
      virtual ~TabbedCrawlerWidget() {}

      // "Overridden" addTab/removeTab that automatically
      // show/hide the tab bar
      int addTab( QWidget* page, const QString& label );
      void removeTab( int index );

    private:
      QTabBar myTabBar_;
};

#endif

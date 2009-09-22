/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CRAWLERWIDGET_H
#define CRAWLERWIDGET_H

#include <QLabel>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "logmainview.h"
#include "filteredview.h"

#include "logdata.h"
#include "logfiltereddata.h"

class CrawlerWidget : public QSplitter
{
    Q_OBJECT

    public:
        CrawlerWidget(QWidget *parent=0);

        bool readFile(const QString &fileName);

    private slots:
        void searchClicked();
        void applyConfiguration();
        void updateFilteredView();
        void filteredViewSelection( int lineNb );

    private:
        LogMainView*    logMainView;
        QWidget*        bottomWindow;
        QLabel*         searchLabel;
        QLineEdit*      searchLineEdit;
        QPushButton*    searchButton;
        FilteredView*   filteredView;
        QLabel*         searchInfoLine;

        QVBoxLayout*    bottomMainLayout;
        QHBoxLayout*    searchLineLayout;

        static LogData  emptyLogData;
        static LogFilteredData  emptyLogFilteredData;

        LogData*        logData;
        LogFilteredData* logFilteredData;
};

#endif

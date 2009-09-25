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

// This file implements the LogMainView concrete class.
// Most of the actual drawing and event management is done in AbstractLogView
// Only behaviour specific to the main (top) view is implemented here.

#include "logmainview.h"

LogMainView::LogMainView(const LogData* newLogData, QWidget* parent)
    : AbstractLogView(newLogData, parent)
{
    filteredData_ = NULL;
}

// Just update our internal record and redraw the view in case
// there is any change in bullet colours.
void LogMainView::useNewFiltering( LogFilteredData* filteredData )
{
    filteredData_ = filteredData;
    update();
}

bool LogMainView::isLineMatching( int lineNumber )
{
    if ( filteredData_ != NULL )
        return filteredData_->isLineInMatchingList( lineNumber );
    else
        return false;
}

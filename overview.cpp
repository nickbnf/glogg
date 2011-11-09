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

// This file implements the Overview class.
// It provides support for drawing the match overview sidebar but
// the actual drawing is done in AbstractLogView which uses this class.

#include "log.h"

#include "overview.h"

Overview::Overview() : matchLines_(), markLines_()
{
    logFilteredData_ = NULL;
    linesInFile_     = 0;
    topLine_         = 0;
    lastLine_        = 0;
    height_          = 0;
    dirty_           = true;
    visible_         = false;
}

Overview::~Overview()
{
}

void Overview::setFilteredData( const LogFilteredData* logFilteredData )
{
    logFilteredData_ = logFilteredData;
}

void Overview::updateData( int totalNbLine )
{
    LOG(logDEBUG) << "OverviewWidget::updateData " << totalNbLine;

    linesInFile_ = totalNbLine;
    dirty_ = true;
}

void Overview::updateView( int height )
{
    // We don't touch the cache if the height hasn't changed
    if ( ( height != height_ ) || ( dirty_ == true ) ) {
        height_ = height;

        recalculatesLines();
    }
}

const QList<int>* Overview::getMatchLines() const
{
    return &matchLines_;
}

const QList<int>* Overview::getMarkLines() const
{
    return &markLines_;
}

std::pair<int,int> Overview::getViewLines() const
{
    int top = 0;
    int bottom = height_ - 1;

    if ( linesInFile_ > 0 ) {
        top = topLine_ * height_ / linesInFile_;
        bottom = lastLine_ * height_ / linesInFile_;
    }

    return std::pair<int,int>(top, bottom);
}

// Update the internal cache
void Overview::recalculatesLines()
{
    LOG(logDEBUG) << "OverviewWidget::recalculatesLines";

    if ( logFilteredData_ != NULL ) {
        matchLines_.clear();

        for ( int i = 0; i < logFilteredData_->getNbLine(); i++ ) {
            int line = (int) logFilteredData_->getMatchingLineNumber( i );
            matchLines_ << ( line * height_ / linesInFile_ );
        }
    }
    else
        LOG(logERROR) << "Overview::recalculatesLines: logFilteredData_ == NULL";

    dirty_ = false;
}

/*
 * Copyright (C) 2011, 2012 Nicolas Bonnefon and other contributors
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

#include "data/logfiltereddata.h"

#include "overview.h"

Overview::Overview() : matchLines_(), markLines_()
{
    logFilteredData_ = NULL;
    linesInFile_     = 0;
    topLine_         = 0;
    nbLines_         = 0;
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

const QVector<Overview::WeightedLine>* Overview::getMatchLines() const
{
    return &matchLines_;
}

const QVector<Overview::WeightedLine>* Overview::getMarkLines() const
{
    return &markLines_;
}

std::pair<int,int> Overview::getViewLines() const
{
    int top = 0;
    int bottom = height_ - 1;

    if ( linesInFile_ > 0 ) {
        top = (int)((qint64)topLine_ * height_ / linesInFile_);
        bottom = (int)((qint64)top + nbLines_ * height_ / linesInFile_);
    }

    return std::pair<int,int>(top, bottom);
}

int Overview::fileLineFromY( int position ) const
{
    int line = (int)((qint64)position * linesInFile_ / height_);

    return line;
}

int Overview::yFromFileLine( int file_line ) const
{
    int position = 0;

    if ( linesInFile_ > 0 )
        position =  (int)((qint64)file_line * height_ / linesInFile_);

    return position;
}

// Update the internal cache
void Overview::recalculatesLines()
{
    LOG(logDEBUG) << "OverviewWidget::recalculatesLines";

    if ( logFilteredData_ != NULL ) {
        matchLines_.clear();
        markLines_.clear();

        if ( linesInFile_ > 0 ) {
            for ( int i = 0; i < logFilteredData_->getNbLine(); i++ ) {
                LogFilteredData::FilteredLineType line_type =
                    logFilteredData_->filteredLineTypeByIndex( i );
                int line = (int) logFilteredData_->getMatchingLineNumber( i );
                int position = (int)( (qint64)line * height_ / linesInFile_ );
                if ( line_type & LogFilteredData::Match ) {
                    if ( ( ! matchLines_.isEmpty() ) && matchLines_.last().position() == position ) {
                        // If the line is already there, we increase its weight
                        matchLines_.last().load();
                    }
                    else {
                        // If not we just add it
                        matchLines_.append( WeightedLine( position ) );
                    }
                }
                else {
                    if ( ( ! markLines_.isEmpty() ) && markLines_.last().position() == position ) {
                        // If the line is already there, we increase its weight
                        markLines_.last().load();
                    }
                    else {
                        // If not we just add it
                        markLines_.append( WeightedLine( position ) );
                    }
                }
            }
        }
    }
    else
        LOG(logDEBUG) << "Overview::recalculatesLines: logFilteredData_ == NULL";

    dirty_ = false;
}

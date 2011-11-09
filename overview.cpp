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

#include "overview.h"

Overview::Overview() : matchLines_(), markLines_()
{
    logFilteredData_ = NULL;
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

void Overview::updateView( int height )
{
    height_ = height;

    matchLines_ << 3;
    matchLines_ << 12;
}

const QList<int>* Overview::getMatchLines() const
{
    return &matchLines_;
}

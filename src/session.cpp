/*
 * Copyright (C) 2013 Nicolas Bonnefon and other contributors
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

#include "session.h"

#include <cassert>
#include <QFileInfo>

#include "viewinterface.h"
#include "data/logdata.h"
#include "data/logfiltereddata.h"

Session::Session()
{
}

Session::~Session()
{
    // FIXME Clean up all the data objects...
}

ViewInterface* Session::open( const std::string& file_name,
        std::function<ViewInterface*()> view_factory )
{
    ViewInterface* view = nullptr;

    QFileInfo fileInfo( file_name.c_str() );
    if ( fileInfo.isReadable() )
    {
        // Create the data objects
        auto log_data          = std::make_shared<LogData>();
        auto log_filtered_data =
            std::shared_ptr<LogFilteredData>( log_data->getNewFilteredData() );

        view = view_factory();
        view->setData( log_data, log_filtered_data );

        // Insert in the hash
        open_files_.insert( { view,
                { file_name,
                  log_data,
                  log_filtered_data,
                  view } } );

        // Start loading the file
        log_data->attachFile( QString( file_name.c_str() ) );
    }
    else {
        // throw
    }

    return view;
}

void Session::getFileInfo( const ViewInterface* view, uint64_t* fileSize,
        uint32_t* fileNbLine, QDateTime* lastModified ) const
{
    const OpenFile* file = findOpenFileFromView( view );

    assert( file );

    *fileSize = file->logData->getFileSize();
    *fileNbLine = file->logData->getNbLine();
    *lastModified = file->logData->getLastModifiedDate();
}


/*
 * Private methods
 */

Session::OpenFile* Session::findOpenFileFromView( const ViewInterface* view )
{
    assert( view );

    OpenFile* file = &( open_files_.at( view ) );

    // OpenfileMap::at might throw out_of_range but since a view MUST always
    // be attached to a file, we don't handle it!

    return file;
}

const Session::OpenFile* Session::findOpenFileFromView( const ViewInterface* view ) const
{
    assert( view );

    const OpenFile* file = &( open_files_.at( view ) );

    // OpenfileMap::at might throw out_of_range but since a view MUST always
    // be attached to a file, we don't handle it!

    return file;
}

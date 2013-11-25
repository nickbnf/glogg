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
        logData_          = std::shared_ptr<LogData>( new LogData() );
        logFilteredData_  =
            std::shared_ptr<LogFilteredData>( logData_->getNewFilteredData() );

        view = view_factory();
        view->setData( logData_, logFilteredData_ );

        // Start loading the file
        logData_->attachFile( QString( file_name.c_str() ) );
    }
    else {
        // throw
    }

    return view;
}
/*
void CrawlerWidget::stopLoading()
{
    logFilteredData_->interruptSearch();
    logData_->interruptLoading();
}

void CrawlerWidget::getFileInfo( qint64* fileSize, int* fileNbLine,
       QDateTime* lastModified ) const
{
    *fileSize = logData_->getFileSize();
    *fileNbLine = logData_->getNbLine();
    *lastModified = logData_->getLastModifiedDate();
}
*/

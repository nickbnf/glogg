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

#ifndef VIEWINTERFACE_H
#define VIEWINTERFACE_H

#include <memory>

class LogData;
class LogFilteredData;

// ViewInterface represents a high-level view on a log file.
class ViewInterface {
  public:
    // Set the log data and filtered data to associate to this view
    // Ownership stay with the caller but is shared
    void setData( std::shared_ptr<LogData> log_data,
            std::shared_ptr<LogFilteredData> filtered_data )
    { doSetData( log_data, filtered_data ); }

    // For save/restore of the context
    /*
    virtual void setViewContext( const ViewContextInterface& view_context ) = 0;
    virtual ViewContextInterface& getViewContext( void ) = 0;
    */

    // To allow polymorphic destruction
    virtual ~ViewInterface() {}

  protected:
    // Virtual functions (using NVI)
    virtual void doSetData( std::shared_ptr<LogData> log_data,
            std::shared_ptr<LogFilteredData> filtered_data ) = 0;
};
#endif

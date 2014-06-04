/*
 * Copyright (C) 2013, 2014 Nicolas Bonnefon and other contributors
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
class SavedSearches;
class QuickFindPattern;

// ViewContextInterface represents the private information
// the concrete view will be able to save and restore.
// It can be marshalled to persistent storage.
class ViewContextInterface {
  public:
    virtual ~ViewContextInterface() {}

    virtual std::string toString() const = 0;
};

// ViewInterface represents a high-level view on a log file.
// This a pure virtual class (interface) which is subclassed
// for each type of view.
class ViewInterface {
  public:
    // Set the log data and filtered data to associate to this view
    // Ownership stay with the caller but is shared
    void setData( std::shared_ptr<LogData> log_data,
            std::shared_ptr<LogFilteredData> filtered_data )
    { doSetData( log_data, filtered_data ); }

    // Set the (shared) quickfind pattern object
    void setQuickFindPattern( std::shared_ptr<QuickFindPattern> qfp )
    { doSetQuickFindPattern( qfp ); }

    // Set the (shared) search history object
    void setSavedSearches( std::shared_ptr<SavedSearches> saved_searches )
    { doSetSavedSearches( saved_searches ); }

    // For save/restore of the context
    void setViewContext( const char* view_context )
    { doSetViewContext( view_context ); }
    // (returned object ownership is transferred to the caller)
    std::shared_ptr<const ViewContextInterface> context( void ) const
    { return doGetViewContext(); }

    // To allow polymorphic destruction
    virtual ~ViewInterface() {}

  protected:
    // Virtual functions (using NVI)
    virtual void doSetData( std::shared_ptr<LogData> log_data,
            std::shared_ptr<LogFilteredData> filtered_data ) = 0;
    virtual void doSetQuickFindPattern(
            std::shared_ptr<QuickFindPattern> qfp ) = 0;
    virtual void doSetSavedSearches(
            std::shared_ptr<SavedSearches> saved_searches ) = 0;
    virtual void doSetViewContext(
            const char* view_context ) = 0;
    virtual std::shared_ptr<const ViewContextInterface>
        doGetViewContext( void ) const = 0;
};
#endif

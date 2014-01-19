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

#ifndef SESSION_H
#define SESSION_H

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <utility>

#include <QDateTime>

#include "quickfindpattern.h"

class ViewInterface;
class LogData;
class LogFilteredData;
class SavedSearches;

// File unreadable error
class FileUnreadableErr {};

// The session is responsible for maintaining the list of open log files
// and their association with Views.
// It also maintains the domain objects which are common to all log files
// (SavedSearches, FileHistory, QFPattern...)
class Session {
  public:
    Session();
    ~Session();

    // No copy/assignment please
    Session( const Session& ) = delete;
    Session& operator =( const Session& ) = delete;

    // Return the view associated to a file if it is open
    // The filename must be strictly identical to trigger a match
    // (no match in case of e.g. relative vs. absolute pathname.
    ViewInterface* getViewIfOpen( const std::string& file_name ) const;
    // Open a new file, starts its asynchronous loading, and construct a new
    // view for it (the caller passes a factory to build the concrete view)
    // The ownership of the view is given to the caller
    // Throw exceptions if the file is already open or if it cannot be open.
    ViewInterface* open( const std::string& file_name,
            std::function<ViewInterface*()> view_factory );
    // Close the file identified by the view passed
    // Throw an exception if it does not exist.
    void close( const ViewInterface* view );

    // Open all the files listed in the stored session
    // (see ::open)
    // returns a vector of pairs (file_name, view) and the index of the
    // current file (or -1 if none).
    std::vector<std::pair<std::string, ViewInterface*>> restore(
            std::function<ViewInterface*()> view_factory,
            int *current_file_index );
    // Save the session to persistent storage. An ordered list of
    // (view, topLine) is passed, this is because only the main window
    // know the order in which the views are presented to the user (it might
    // have changed since file were opened).
    void save(
            std::vector<std::pair<const ViewInterface*, uint64_t>> view_list );

    // Get the file name for the passed view.
    std::string getFilename( const ViewInterface* view ) const;
    // Get the size (in bytes) and number of lines in the current file.
    // The file is identified by the view attached to it.
    void getFileInfo( const ViewInterface* view, uint64_t* fileSize,
            uint32_t* fileNbLine, QDateTime* lastModified ) const;
    // Get a (non-const) reference to the QuickFind pattern.
    std::shared_ptr<QuickFindPattern> getQuickFindPattern() const
    { return quickFindPattern_; }

  private:
    struct OpenFile {
        std::string fileName;
        std::shared_ptr<LogData> logData;
        std::shared_ptr<LogFilteredData> logFilteredData;
        ViewInterface* view;
    };

    // Open a file without checking if it is existing/readable
    ViewInterface* openAlways( const std::string& file_name,
            std::function<ViewInterface*()> view_factory );
    // Find an open file from its associated view
    OpenFile* findOpenFileFromView( const ViewInterface* view );
    const OpenFile* findOpenFileFromView( const ViewInterface* view ) const;

    // List of open files
    typedef std::unordered_map<const ViewInterface*, OpenFile> OpenFileMap;
    OpenFileMap openFiles_;

    // Global search history
    std::shared_ptr<SavedSearches> savedSearches_;

    // Global quickfind pattern
    std::shared_ptr<QuickFindPattern> quickFindPattern_;
};

#endif

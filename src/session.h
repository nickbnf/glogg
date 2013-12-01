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

#ifndef SESSION_H
#define SESSION_H

#include <memory>
#include <unordered_map>
#include <string>

#include <QDateTime>

class ViewInterface;
class LogData;
class LogFilteredData;

// The session is responsible for maintaining the list of open log files
// and their association with Views.
// It also maintains the domain objects which are common to all log files
// (SearchHistory, FileHistory, QFPattern...)
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
    // Get the size (in bytes) and number of lines in the current file.
    // The file is identified by the view attached to it.
    void getFileInfo(  const ViewInterface* view, uint64_t* fileSize,
            uint32_t* fileNbLine, QDateTime* lastModified ) const;

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
    OpenFileMap open_files_;
};

#endif

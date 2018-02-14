/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2017 Nicolas Bonnefon and other contributors
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

// This file implements LogFilteredData
// It stores a pointer to the LogData that created it,
// so should always be destroyed before the LogData.

#include "log.h"

#include <QString>
#include <cassert>
#include <limits>

#include "utils.h"
#include "logdata.h"
#include "marks.h"
#include "logfiltereddata.h"

// Creates an empty set. It must be possible to display it without error.
// FIXME
LogFilteredData::LogFilteredData() : AbstractLogData(),
    matching_lines_(),
    currentRegExp_(),
    visibility_(),
    filteredItemsCache_(),
    workerThread_( nullptr ),
    marks_()
{
    /* Prevent any more searching */
    maxLength_ = 0;
    maxLengthMarks_ = 0;
    searchDone_ = true;
    visibility_ = MarksAndMatches;

    filteredItemsCacheDirty_ = true;
}

// Usual constructor: just copy the data, the search is started by runSearch()
LogFilteredData::LogFilteredData( const LogData* logData )
    : AbstractLogData(),
    matching_lines_( SearchResultArray() ),
    currentRegExp_(),
    visibility_(),
    filteredItemsCache_(),
    workerThread_( logData ),
    marks_()
{
    // Starts with an empty result list
    maxLength_ = 0;
    maxLengthMarks_ = 0;
    nbLinesProcessed_ = 0;

    sourceLogData_ = logData;

    searchDone_ = false;

    visibility_ = MarksAndMatches;

    filteredItemsCacheDirty_ = true;

    // Forward the update signal
    connect( &workerThread_, SIGNAL( searchProgressed( int, int, qint64 ) ),
            this, SLOT( handleSearchProgressed( int, int, qint64 ) ) );

    // Starts the worker thread
    workerThread_.start();
}

LogFilteredData::~LogFilteredData()
{
    // FIXME
    // workerThread_.stop();
}

//
// Public functions
//

// Run the search and send newDataAvailable() signals.
void LogFilteredData::runSearch( const QRegularExpression& regExp )
{
    LOG(logDEBUG) << "Entering runSearch";

    clearSearch();
    currentRegExp_ = regExp;

    workerThread_.search( currentRegExp_ );
}

void LogFilteredData::updateSearch()
{
    LOG(logDEBUG) << "Entering updateSearch";

    workerThread_.updateSearch( currentRegExp_, nbLinesProcessed_ );
}

void LogFilteredData::interruptSearch()
{
    LOG(logDEBUG) << "Entering interruptSearch";

    workerThread_.interrupt();
}

void LogFilteredData::clearSearch()
{
    currentRegExp_ = QRegularExpression();
    matching_lines_.clear();
    maxLength_        = 0;
    nbLinesProcessed_ = 0;
    filteredItemsCacheDirty_ = true;
}

qint64 LogFilteredData::getMatchingLineNumber( int matchNum ) const
{
    qint64 matchingLine = findLogDataLine( matchNum );

    return matchingLine;
}

// Scan the list for the 'lineNumber' passed
bool LogFilteredData::isLineInMatchingList( qint64 lineNumber )
{
    int index;                                    // Not used
    return lookupLineNumber<SearchResultArray>(
            matching_lines_, lineNumber, &index);
}

int LogFilteredData::getLineIndexNumber( quint64 lineNumber ) const
{
    int lineIndex = findFilteredLine( lineNumber );
    return lineIndex;
}

LineNumber LogFilteredData::getNbTotalLines() const
{
    return sourceLogData_->getNbLine();
}

LineNumber LogFilteredData::getNbMatches() const
{
    return matching_lines_.size();
}

LineNumber LogFilteredData::getNbMarks() const
{
    return marks_.size();
}

LogFilteredData::FilteredLineType
    LogFilteredData::filteredLineTypeByIndex( int index ) const
{
    // If we are only showing one type, the line is there because
    // it is of this type.
    if ( visibility_ == MatchesOnly )
        return Match;
    else if ( visibility_ == MarksOnly )
        return Mark;
    else {
        // If it is MarksAndMatches, we have to look.
        // Regenerate the cache if needed
        if ( filteredItemsCacheDirty_ )
            regenerateFilteredItemsCache();

        return filteredItemsCache_[ index ].type();
    }
}

// Delegation to our Marks object

void LogFilteredData::addMark( qint64 line, QChar mark )
{
    if ( ( line >= 0 ) && ( line < sourceLogData_->getNbLine() ) ) {
        marks_.addMark( line, mark );
        maxLengthMarks_ = qMax( maxLengthMarks_,
                sourceLogData_->getLineLength( line ) );
        filteredItemsCacheDirty_ = true;
    }
    else
        LOG(logERROR) << "LogFilteredData::addMark\
 trying to create a mark outside of the file.";
}

qint64 LogFilteredData::getMark( QChar mark ) const
{
    return marks_.getMark( mark );
}

bool LogFilteredData::isLineMarked( qint64 line ) const
{
    return marks_.isLineMarked( line );
}

qint64 LogFilteredData::getMarkAfter( qint64 line ) const
{
    qint64 marked_line = -1;

    for ( auto i = marks_.begin(); i != marks_.end(); ++i ) {
        if ( i->lineNumber() > line ) {
            marked_line = i->lineNumber();
            break;
        }
    }

    return marked_line;
}

qint64 LogFilteredData::getMarkBefore( qint64 line ) const
{
    qint64 marked_line = -1;

    for ( auto i = marks_.begin(); i != marks_.end(); ++i ) {
        if ( i->lineNumber() >= line ) {
            break;
        }
        marked_line = i->lineNumber();
    }

    return marked_line;
}

void LogFilteredData::deleteMark( QChar mark )
{
    marks_.deleteMark( mark );
    filteredItemsCacheDirty_ = true;

    // FIXME: maxLengthMarks_
}

void LogFilteredData::deleteMark( qint64 line )
{
    marks_.deleteMark( line );
    filteredItemsCacheDirty_ = true;

    // Now update the max length if needed
    if ( sourceLogData_->getLineLength( line ) >= maxLengthMarks_ ) {
        LOG(logDEBUG) << "deleteMark recalculating longest mark";
        maxLengthMarks_ = 0;
        for ( Marks::const_iterator i = marks_.begin();
                i != marks_.end(); ++i ) {
            LOG(logDEBUG) << "line " << i->lineNumber();
            maxLengthMarks_ = qMax( maxLengthMarks_,
                    sourceLogData_->getLineLength( i->lineNumber() ) );
        }
    }
}

void LogFilteredData::clearMarks()
{
    marks_.clear();
    filteredItemsCacheDirty_ = true;
    maxLengthMarks_ = 0;
}

void LogFilteredData::setVisibility( Visibility visi )
{
    visibility_ = visi;
}

//
// Slots
//
void LogFilteredData::handleSearchProgressed( int nbMatches, int progress, qint64 initial_position )
{
    LOG(logDEBUG) << "LogFilteredData::handleSearchProgressed matches="
        << nbMatches << " progress=" << progress;

    // searchDone_ = true;
    workerThread_.getSearchResult( &maxLength_, &matching_lines_, &nbLinesProcessed_ );
    filteredItemsCacheDirty_ = true;

    emit searchProgressed( nbMatches, progress, initial_position );
}

LineNumber LogFilteredData::findLogDataLine( LineNumber lineNum ) const
{
    LineNumber line = std::numeric_limits<LineNumber>::max();
    if ( visibility_ == MatchesOnly ) {
        if ( lineNum < matching_lines_.size() ) {
            line = matching_lines_[lineNum].lineNumber();
        }
        else {
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum;
        }
    }
    else if ( visibility_ == MarksOnly ) {
        if ( lineNum < marks_.size() )
            line = marks_.getLineMarkedByIndex( lineNum );
        else
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum;
    }
    else {
        // Regenerate the cache if needed
        if ( filteredItemsCacheDirty_ )
            regenerateFilteredItemsCache();

        if ( lineNum < filteredItemsCache_.size() )
            line = filteredItemsCache_[ lineNum ].lineNumber();
        else
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum;
    }

    return line;
}

LineNumber LogFilteredData::findFilteredLine( LineNumber lineNum ) const
{
    LineNumber lineIndex = std::numeric_limits<LineNumber>::max();

    if ( visibility_ == MatchesOnly ) {
        lineIndex = lookupLineNumber( matching_lines_.begin(),
                                      matching_lines_.end(),
                                      lineNum );
    }
    else if ( visibility_ == MarksOnly ) {
        lineIndex = lookupLineNumber( marks_.begin(),
                                      marks_.end(),
                                      lineNum );
    }
    else {
      // Regenerate the cache if needed
        if ( filteredItemsCacheDirty_ ) {
            regenerateFilteredItemsCache();
        }

        lineIndex = lookupLineNumber( filteredItemsCache_.begin(),
                                      filteredItemsCache_.end(),
                                      lineNum );
    }

    return lineIndex;
}

// Implementation of the virtual function.
QString LogFilteredData::doGetLineString( qint64 lineNum ) const
{
    qint64 line = findLogDataLine( lineNum );

    QString string = sourceLogData_->getLineString( line );
    return string;
}

// Implementation of the virtual function.
QString LogFilteredData::doGetExpandedLineString( qint64 lineNum ) const
{
    qint64 line = findLogDataLine( lineNum );

    QString string = sourceLogData_->getExpandedLineString( line );
    return string;
}

// Implementation of the virtual function.
QStringList LogFilteredData::doGetLines( qint64 first_line, int number ) const
{
    QStringList list;

    for ( int i = first_line; i < first_line + number; i++ ) {
        list.append( doGetLineString( i ) );
    }

    return list;
}

// Implementation of the virtual function.
QStringList LogFilteredData::doGetExpandedLines( qint64 first_line, int number ) const
{
    QStringList list;

    for ( int i = first_line; i < first_line + number; i++ ) {
        list.append( doGetExpandedLineString( i ) );
    }

    return list;
}

// Implementation of the virtual function.
qint64 LogFilteredData::doGetNbLine() const
{
    qint64 nbLines;

    if ( visibility_ == MatchesOnly )
        nbLines = matching_lines_.size();
    else if ( visibility_ == MarksOnly )
        nbLines = marks_.size();
    else {
        // Regenerate the cache if needed (hopefully most of the time
        // it won't be necessarily)
        if ( filteredItemsCacheDirty_ )
            regenerateFilteredItemsCache();
        nbLines = filteredItemsCache_.size();
    }

    return nbLines;
}

// Implementation of the virtual function.
int LogFilteredData::doGetMaxLength() const
{
    int max_length;

    if ( visibility_ == MatchesOnly )
        max_length = maxLength_;
    else if ( visibility_ == MarksOnly )
        max_length = maxLengthMarks_;
    else
        max_length = qMax( maxLength_, maxLengthMarks_ );

    return max_length;
}

// Implementation of the virtual function.
int LogFilteredData::doGetLineLength( qint64 lineNum ) const
{
    qint64 line = findLogDataLine( lineNum );
    return sourceLogData_->getExpandedLineString( line ).length();
}

void LogFilteredData::doSetDisplayEncoding( Encoding encoding )
{
    LOG(logDEBUG) << "AbstractLogData::setDisplayEncoding: " << static_cast<int>( encoding );
}

void LogFilteredData::doSetMultibyteEncodingOffsets( int, int )
{
}

// TODO: We might be a bit smarter and not regenerate the whole thing when
// e.g. stuff is added at the end of the search.
void LogFilteredData::regenerateFilteredItemsCache() const
{
    LOG(logDEBUG) << "regenerateFilteredItemsCache";

    filteredItemsCache_.clear();
    filteredItemsCache_.reserve( matching_lines_.size() + marks_.size() );
    // (it's an overestimate but probably not by much so it's fine)

    auto i = matching_lines_.cbegin();
    Marks::const_iterator j = marks_.begin();

    while ( ( i != matching_lines_.cend() ) || ( j != marks_.end() ) ) {
        qint64 next_mark =
            ( j != marks_.end() ) ? j->lineNumber() : std::numeric_limits<qint64>::max();
        qint64 next_match =
            ( i != matching_lines_.cend() ) ? i->lineNumber() : std::numeric_limits<qint64>::max();
        // We choose a Mark over a Match if a line is both, just an arbitrary choice really.
        if ( next_mark <= next_match ) {
            // LOG(logDEBUG) << "Add mark at " << next_mark;
            filteredItemsCache_.push_back( FilteredItem( next_mark, Mark ) );
            if ( j != marks_.end() )
                ++j;
            if ( ( next_mark == next_match ) && ( i != matching_lines_.cend() ) )
                ++i;  // Case when it's both match and mark.
        }
        else {
            // LOG(logDEBUG) << "Add match at " << next_match;
            filteredItemsCache_.push_back( FilteredItem( next_match, Match ) );
            if ( i != matching_lines_.cend() )
                ++i;
        }
    }

    filteredItemsCacheDirty_ = false;

    LOG(logDEBUG) << "finished regenerateFilteredItemsCache";
}

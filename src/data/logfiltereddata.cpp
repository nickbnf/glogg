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
    removeAllFromFilteredItemsCache( Match );
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
        return filteredItemsCache_[ index ].type();
    }
}

// Delegation to our Marks object

void LogFilteredData::addMark( qint64 line, QChar mark )
{
    if ( ( line >= 0 ) && ( line < sourceLogData_->getNbLine() ) ) {
        int index = marks_.addMark( line, mark );
        maxLengthMarks_ = qMax( maxLengthMarks_,
                sourceLogData_->getLineLength( line ) );
        insertIntoFilteredItemsCache( index, FilteredItem{ static_cast<LineNumber>( line ), Mark } );
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
    int index = marks_.findMark( mark );
    qint64 line = marks_.getLineMarkedByIndex( index );
    marks_.deleteMarkAt( index );

    if ( line < 0 ) {
        // LOG(logWARNING)?
        return;
    }

    updateMaxLengthMarks( line );
    removeFromFilteredItemsCache( index, FilteredItem{ static_cast<LineNumber>( line ), Mark } );
}

void LogFilteredData::deleteMark( qint64 line )
{
    int index = marks_.deleteMark( line );

    updateMaxLengthMarks( line );
    removeFromFilteredItemsCache( index, FilteredItem{ static_cast<LineNumber>( line ), Mark } );
}

void LogFilteredData::updateMaxLengthMarks( qint64 removed_line )
{
    if ( removed_line < 0 ) {
        LOG(logWARNING) << "updateMaxLengthMarks called with negative line-number";
        return;
    }
    // Now update the max length if needed
    if ( sourceLogData_->getLineLength( removed_line ) >= maxLengthMarks_ ) {
        LOG(logDEBUG) << "deleteMark recalculating longest mark";
        maxLengthMarks_ = 0;
        for ( auto& mark : marks_ ) {
            LOG(logDEBUG) << "line " << mark.lineNumber();
            maxLengthMarks_ = qMax( maxLengthMarks_,
                    sourceLogData_->getLineLength( mark.lineNumber() ) );
        }
    }
}

void LogFilteredData::clearMarks()
{
    marks_.clear();
    maxLengthMarks_ = 0;
    removeAllFromFilteredItemsCache( Mark );
}

void LogFilteredData::setVisibility( Visibility visi )
{
    visibility_ = visi;

    if ( visibility_ == MarksAndMatches )
        regenerateFilteredItemsCache();
}

//
// Slots
//
void LogFilteredData::handleSearchProgressed( int nbMatches, int progress, qint64 initial_position )
{
    using std::begin;
    using std::end;
    using std::next;

    LOG(logDEBUG) << "LogFilteredData::handleSearchProgressed matches="
        << nbMatches << " progress=" << progress;

    // searchDone_ = true;
    assert( nbMatches >= 0 );

    size_t start_index = matching_lines_.size();

    workerThread_.updateSearchResult( &maxLength_, &matching_lines_, &nbLinesProcessed_ );

    insertMatchesIntoFilteredItemsCache( start_index );

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

void LogFilteredData::regenerateFilteredItemsCache() const
{
    LOG(logDEBUG) << "regenerateFilteredItemsCache";

    if ( filteredItemsCache_.size() > 0 ) {
        // the cache was not invalidated, so we can keep it
        LOG(logDEBUG) << "cache was not invalidated";
        return;
    }

    filteredItemsCache_.reserve( matching_lines_.size() + marks_.size() );
    // (it's an overestimate but probably not by much so it's fine)

    auto i = matching_lines_.cbegin();
    Marks::const_iterator j = marks_.begin();

    while ( ( i != matching_lines_.cend() ) || ( j != marks_.end() ) ) {
        qint64 next_mark =
            ( j != marks_.end() ) ? j->lineNumber() : std::numeric_limits<qint64>::max();
        qint64 next_match =
            ( i != matching_lines_.cend() ) ? i->lineNumber() : std::numeric_limits<qint64>::max();
        FilteredLineType type = static_cast<FilteredLineType>( 0 );
        LineNumber line;
        if ( next_mark <= next_match ) {
            // LOG(logDEBUG) << "Add mark at " << next_mark;
            type |= Mark;
            line = next_mark;
            ++j;
        }
        if ( next_mark >= next_match ) {
            // LOG(logDEBUG) << "Add match at " << next_match;
            type |= Match;
            line = next_match;
            ++i;
        }
        filteredItemsCache_.push_back( FilteredItem( line, type ) );
    }

    LOG(logDEBUG) << "finished regenerateFilteredItemsCache";
}

void LogFilteredData::insertIntoFilteredItemsCache( size_t insert_index, FilteredItem item )
{
    using std::begin;
    using std::end;
    using std::next;

    if ( visibility_ != MarksAndMatches ) {
        // this is invalidated and will be regenerated when we need it
        filteredItemsCache_.clear();
        LOG(logDEBUG) << "cache is invalidated";
    }

    // Search for the corresponding index.
    // We can start the search from insert_index, since lineNumber >= index is always true.
    auto found = std::lower_bound( next( begin( filteredItemsCache_ ), insert_index ), end( filteredItemsCache_ ), item );
    if ( found == end( filteredItemsCache_ ) || found->lineNumber() > item.lineNumber() ) {
        filteredItemsCache_.insert( found, item );
    } else {
        assert( found->lineNumber() == item.lineNumber() );
        found->add( item.type() );
    }
}

void LogFilteredData::insertIntoFilteredItemsCache( FilteredItem item )
{
    insertIntoFilteredItemsCache( 0, item );
}

void LogFilteredData::insertMatchesIntoFilteredItemsCache( size_t start_index )
{
    using std::begin;
    using std::end;
    using std::next;

    assert( start_index <= matching_lines_.size() );

    if ( visibility_ != MarksAndMatches ) {
        // this is invalidated and will be regenerated when we need it
        filteredItemsCache_.clear();
        LOG(logDEBUG) << "cache is invalidated";
        return;
    }

    assert( start_index <= filteredItemsCache_.size() );

    filteredItemsCache_.reserve( matching_lines_.size() + marks_.size() );
    // (it's an overestimate but probably not by much so it's fine)

    // Search for the corresponding index.
    // We can start the search from insert_index, since lineNumber >= index is always true.
    auto filteredIt = next( begin( filteredItemsCache_ ), start_index );
    for ( auto matchesIt = next( begin( matching_lines_ ), start_index ); matchesIt != end( matching_lines_ ); ++matchesIt ) {
        FilteredItem item{ matchesIt->lineNumber(), Match };
        filteredIt = std::lower_bound( filteredIt, end( filteredItemsCache_ ), item );
        if ( filteredIt == end( filteredItemsCache_ ) || filteredIt->lineNumber() > item.lineNumber() ) {
            filteredIt = filteredItemsCache_.insert( filteredIt, item );
        } else {
            assert( filteredIt->lineNumber() == matchesIt->lineNumber() );
            filteredIt->add( item.type() );
        }
    }
}

void LogFilteredData::removeFromFilteredItemsCache( size_t remove_index, FilteredItem item )
{
    using std::begin;
    using std::distance;
    using std::end;
    using std::next;

    if ( visibility_ != MarksAndMatches ) {
        // this is invalidated and will be regenerated when we need it
        filteredItemsCache_.clear();
        LOG(logDEBUG) << "cache is invalidated";
        return;
    }

    // Search for the corresponding index.
    // We can start the search from remove_index, since lineNumber >= index is always true.
    auto found = std::equal_range( next( begin( filteredItemsCache_ ), remove_index ), end( filteredItemsCache_ ), item );
    if( found.first == end( filteredItemsCache_ ) ) {
        LOG(logERROR) << "Attempt to remove line " << item.lineNumber() << " from filteredItemsCache_ failed, since it was not found";
        return;
    }

    if ( distance( found.first, found.second ) > 1 ) {
        LOG(logERROR) << "Multiple matches found for line " << item.lineNumber() << " in filteredItemsCache_";
        // FIXME: collapse them?
    }

    if ( !found.first->remove( item.type() ) ){
        filteredItemsCache_.erase( found.first );
    }
}

void LogFilteredData::removeAllFromFilteredItemsCache( FilteredLineType type )
{
    using std::begin;
    using std::end;

    if ( visibility_ != MarksAndMatches ) {
        // this is invalidated and will be regenerated when we need it
        filteredItemsCache_.clear();
        LOG(logDEBUG) << "cache is invalidated";
        return;
    }

    auto erase_begin = std::remove_if( begin( filteredItemsCache_ ), end( filteredItemsCache_ ), [type]( FilteredItem& item ) { return !item.remove( type ); } );
    filteredItemsCache_.erase( erase_begin, end( filteredItemsCache_ ) );
}

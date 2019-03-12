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

/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

// This file implements LogFilteredData
// It stores a pointer to the LogData that created it,
// so should always be destroyed before the LogData.

#include "log.h"

#include <QString>
#include <cassert>
#include <limits>
#include <utility>

#include "logdata.h"
#include "marks.h"
#include "logfiltereddata.h"

#include "configuration.h"

namespace {

}

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
    maxLength_ = 0_length;
    maxLengthMarks_ = 0_length;
    nbLinesProcessed_ = 0_lcount;
    visibility_ = Visibility::MarksAndMatches;
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
    maxLength_ = 0_length;
    maxLengthMarks_ = 0_length;
    nbLinesProcessed_ = 0_lcount;

    sourceLogData_ = logData;

    visibility_ = Visibility::MarksAndMatches;

    // Forward the update signal
    connect( &workerThread_, &LogFilteredDataWorkerThread::searchProgressed,
            this, &LogFilteredData::handleSearchProgressed );

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

void LogFilteredData::runSearch(const QRegularExpression& regExp)
{
    runSearch( regExp, 0_lnum, LineNumber( getNbTotalLines().get() ) );
}

// Run the search and send newDataAvailable() signals.
void LogFilteredData::runSearch(const QRegularExpression& regExp,
                                LineNumber startLine, LineNumber endLine)
{
    LOG(logDEBUG) << "Entering runSearch";

    const auto& config = Persistable::get<Configuration>();


    clearSearch();
    currentRegExp_ = regExp;
    currentSearchKey_ = makeCacheKey( regExp, startLine, endLine );

    bool shouldRunSearch = true;
    if ( config.useSearchResultsCache() ) {
        const auto cachedResults = searchResultsCache_.find( currentSearchKey_ );
        if ( cachedResults != std::end( searchResultsCache_ ) ) {
            LOG(logDEBUG) << "Got result from cache";
            shouldRunSearch = false;
            matching_lines_ = cachedResults.value().matching_lines;
            maxLength_ = cachedResults.value().maxLength;
            emit searchProgressed(
                        LinesCount( static_cast<LinesCount::UnderlyingType>( matching_lines_.size() ) ),
                        100,
                        startLine );
        }
    }

    if ( shouldRunSearch ) {
        workerThread_.search( currentRegExp_, startLine, endLine );
    }
}

void LogFilteredData::updateSearch(LineNumber startLine, LineNumber endLine)
{
    LOG(logDEBUG) << "Entering updateSearch";

    currentSearchKey_ = makeCacheKey( currentRegExp_, startLine, endLine );
    workerThread_.updateSearch( currentRegExp_, startLine, endLine,
                                LineNumber( nbLinesProcessed_.get() ) );
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
    maxLength_        = 0_length;
    nbLinesProcessed_ = 0_lcount;
    removeAllFromFilteredItemsCache( FilteredLineTypeFlags::Match );
}

LineNumber LogFilteredData::getMatchingLineNumber( LineNumber matchNum ) const
{
    return findLogDataLine( matchNum );
}

LineNumber LogFilteredData::getLineIndexNumber( LineNumber lineNumber ) const
{
    return findFilteredLine( lineNumber );
}

// Scan the list for the 'lineNumber' passed
bool LogFilteredData::isLineInMatchingList( LineNumber lineNumber )
{
    uint32_t index;                                    // Not used
    return lookupLineNumber<SearchResultArray>(
            matching_lines_, lineNumber, index);
}


LinesCount LogFilteredData::getNbTotalLines() const
{
    return sourceLogData_->getNbLine();
}

LinesCount LogFilteredData::getNbMatches() const
{
    return LinesCount( static_cast<LinesCount::UnderlyingType>( matching_lines_.size() ) );
}

LinesCount LogFilteredData::getNbMarks() const
{
    return LinesCount(marks_.size());
}

LogFilteredData::FilteredLineType
    LogFilteredData::filteredLineTypeByIndex( LineNumber index ) const
{
    // If we are only showing one type, the line is there because
    // it is of this type.
    if ( visibility_ == Visibility::MatchesOnly )
        return FilteredLineTypeFlags::Match;
    else if ( visibility_ == Visibility::MarksOnly )
        return FilteredLineTypeFlags::Mark;
    else {
        auto type = filteredItemsCache_[ index.get() ].type();
        assert( !!type );
        return type;
    }
}

// Delegation to our Marks object

void LogFilteredData::addMark( LineNumber line, QChar mark )
{
    if ( line < sourceLogData_->getNbLine() ) {
        marks_.addMark( line, mark );
        maxLengthMarks_ = qMax( maxLengthMarks_,
                sourceLogData_->getLineLength( line ) );
        insertIntoFilteredItemsCache( { static_cast<LineNumber>( line ), FilteredLineTypeFlags::Mark } );
    }
    else
        LOG(logERROR) << "LogFilteredData::addMark\
 trying to create a mark outside of the file.";
}

LineNumber LogFilteredData::getMark( QChar mark ) const
{
    return marks_.getMark( mark );
}

bool LogFilteredData::isLineMarked( LineNumber line ) const
{
    return marks_.isLineMarked( line );
}

OptionalLineNumber LogFilteredData::getMarkAfter( LineNumber line ) const
{
    OptionalLineNumber marked_line;

    for ( const auto& mark : marks_ ) {
        if ( mark.lineNumber() > line ) {
            marked_line = mark.lineNumber();
            break;
        }
    }

    return marked_line;
}

OptionalLineNumber LogFilteredData::getMarkBefore( LineNumber line ) const
{
    OptionalLineNumber marked_line;

    for ( const auto& mark : marks_ ) {
        if ( mark.lineNumber() >= line ) {
            break;
        }
        marked_line = mark.lineNumber();
    }

    return marked_line;
}

void LogFilteredData::deleteMark( QChar mark )
{
    deleteMark( marks_.getMark( mark ) );
}

void LogFilteredData::deleteMark( LineNumber line )
{
    if ( line < 0_lnum ) {
        return;
    }

    marks_.deleteMark( line );

    updateMaxLengthMarks( line );
    removeFromFilteredItemsCache( { static_cast<LineNumber>( line ), FilteredLineTypeFlags::Mark } );
}

void LogFilteredData::updateMaxLengthMarks( LineNumber removed_line )
{
    if ( removed_line < 0_lnum ) {
        LOG(logWARNING) << "updateMaxLengthMarks called with negative line-number";
        return;
    }
    // Now update the max length if needed
    if ( sourceLogData_->getLineLength( removed_line ) >= maxLengthMarks_ ) {
        LOG(logDEBUG) << "deleteMark recalculating longest mark";
        maxLengthMarks_ = 0_length;
        for ( const auto& mark : marks_ ) {
            LOG(logDEBUG) << "line " << mark.lineNumber();
            maxLengthMarks_ = qMax( maxLengthMarks_,
                    sourceLogData_->getLineLength( mark.lineNumber() ) );
        }
    }
}

void LogFilteredData::clearMarks()
{
    marks_.clear();
    maxLengthMarks_ = 0_length;
    removeAllFromFilteredItemsCache( FilteredLineTypeFlags::Mark );
}

QList<LineNumber> LogFilteredData::getMarks() const
{
    QList<LineNumber> markedLines;
    for ( const auto& m : marks_ ) {
        markedLines.append( m.lineNumber() );
    }
    return markedLines;
}

void LogFilteredData::setVisibility( Visibility visi )
{
    visibility_ = visi;

    if ( visibility_ == Visibility::MarksAndMatches )
        regenerateFilteredItemsCache();
}

//
// Slots
//
void LogFilteredData::handleSearchProgressed( LinesCount nbMatches, int progress, LineNumber initialLine )
{
    using std::begin;
    using std::end;
    using std::next;

    const auto& config = Persistable::get<Configuration>();

    LOG(logDEBUG) << "LogFilteredData::handleSearchProgressed matches="
        << nbMatches << " progress=" << progress;

    assert( nbMatches >= 0_lcount );

    size_t start_index = matching_lines_.size();
    //FIXME: klogg has multi-threaded search
    start_index = 0;
    matching_lines_.clear();

    workerThread_.updateSearchResult( &maxLength_, &matching_lines_, &nbLinesProcessed_ );

    assert( matching_lines_.size() >= start_index );

    filteredItemsCache_.reserve( matching_lines_.size() + marks_.size() );
    // (it's an overestimate but probably not by much so it's fine)

    for ( auto it = next( begin( matching_lines_ ), start_index ); it != end( matching_lines_ ); ++it ) {
        insertIntoFilteredItemsCache( { it->lineNumber(), FilteredLineTypeFlags::Match } );
    }

    if ( progress == 100 && config.useSearchResultsCache()
         && nbLinesProcessed_.get() == getExpectedSearchEnd( currentSearchKey_ ).get() ) {

        const auto maxCacheLines = config.searchResultsCacheLines();

        if ( matching_lines_.size() > maxCacheLines ) {
            LOG(logDEBUG) << "LogFilteredData: too many matches to place in cache";
        }
        else if ( !config.useSearchResultsCache() ) {
            LOG(logDEBUG) << "LogFilteredData: search results cache disabled by configs";
        }
        else {
            LOG(logDEBUG) << "LogFilteredData: caching results for pattern "
                          << currentRegExp_.pattern().toStdString();

            searchResultsCache_[ currentSearchKey_ ] = { matching_lines_, maxLength_ };

            auto cacheSize = std::accumulate(
                searchResultsCache_.cbegin(), searchResultsCache_.cend(), std::size_t{},
                []( auto val, const auto& cachedResults )
                {
                    return val + cachedResults.matching_lines.size();
                });

            LOG(logDEBUG) << "LogFilteredData: cache size " << cacheSize;

            auto cachedResult = std::begin( searchResultsCache_ );
            while( cachedResult != std::end( searchResultsCache_ )
                  && cacheSize > maxCacheLines ) {

                if ( cachedResult.key() == currentSearchKey_ ) {
                    ++cachedResult;
                    continue;
                }

                cacheSize -= cachedResult.value().matching_lines.size();
                cachedResult = searchResultsCache_.erase( cachedResult );
            }
        }
    }

    emit searchProgressed( nbMatches, progress, initialLine );
}

LineNumber LogFilteredData::findLogDataLine( LineNumber lineNum ) const
{
    auto line = maxValue<LineNumber>();
    if ( visibility_ == Visibility::MatchesOnly ) {
        if ( lineNum.get() < matching_lines_.size() ) {
            line = matching_lines_[lineNum.get()].lineNumber();
        }
        else {
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum
                          << " matches size " << matching_lines_.size();
        }
    }
    else if ( visibility_ == Visibility::MarksOnly ) {
        if ( lineNum.get() < marks_.size() )
            line = marks_.getLineMarkedByIndex( lineNum.get() );
        else
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum
                          << " marks size " << marks_.size();
    }
    else {
        assert( visibility_ == Visibility::MarksAndMatches );

        if ( lineNum.get() < filteredItemsCache_.size() )
            line = filteredItemsCache_[ lineNum.get() ].lineNumber();
        else
            LOG(logERROR) << "Index too big in LogFilteredData: " << lineNum
                          << " cache size " << filteredItemsCache_.size();
    }

    return line;
}

LineNumber LogFilteredData::findFilteredLine( LineNumber lineNum ) const
{
    auto lineIndex = maxValue<LineNumber>();

    if ( visibility_ == Visibility::MatchesOnly ) {
        lineIndex = lookupLineNumber( matching_lines_.begin(),
                                      matching_lines_.end(),
                                      lineNum );
    }
    else if ( visibility_ == Visibility::MarksOnly ) {
        lineIndex = lookupLineNumber( marks_.begin(),
                                      marks_.end(),
                                      lineNum );
    }
    else {
        assert( visibility_ == Visibility::MarksAndMatches );

        lineIndex = lookupLineNumber( filteredItemsCache_.begin(),
                                      filteredItemsCache_.end(),
                                      lineNum );

    }

    return lineIndex;
}

// Implementation of the virtual function.
QString LogFilteredData::doGetLineString( LineNumber lineNum ) const
{
    const auto line = findLogDataLine( lineNum );
    return sourceLogData_->getLineString( line );
}

// Implementation of the virtual function.
QString LogFilteredData::doGetExpandedLineString( LineNumber lineNum ) const
{
    const auto line = findLogDataLine( lineNum );
    return sourceLogData_->getExpandedLineString( line );
}

// Implementation of the virtual function.
std::vector<QString> LogFilteredData::doGetLines( LineNumber first_line, LinesCount number ) const
{
    std::vector<QString> list;
    list.reserve( number.get() );

    for ( auto i = first_line; i < first_line + number; ++i ) {
        list.emplace_back( doGetLineString( i ) );
    }

    return list;
}

// Implementation of the virtual function.
std::vector<QString> LogFilteredData::doGetExpandedLines( LineNumber first_line, LinesCount number ) const
{
    std::vector<QString> list;
    list.reserve( number.get() );

    for ( auto i = first_line; i < first_line + number; ++i ) {
        list.emplace_back( doGetExpandedLineString( i ) );
    }

    return list;
}

// Implementation of the virtual function.
LinesCount LogFilteredData::doGetNbLine() const
{
    size_t nbLines {};

    if ( visibility_ == Visibility::MatchesOnly )
        nbLines = matching_lines_.size();
    else if ( visibility_ == Visibility::MarksOnly )
        nbLines = marks_.size();
    else {
        assert( visibility_ == Visibility::MarksAndMatches );

        nbLines = filteredItemsCache_.size();
    }

    return LinesCount( static_cast<LinesCount::UnderlyingType>( nbLines ) );
}

// Implementation of the virtual function.
LineLength LogFilteredData::doGetMaxLength() const
{
    LineLength max_length;

    if ( visibility_ == Visibility::MatchesOnly )
        max_length = maxLength_;
    else if ( visibility_ == Visibility::MarksOnly )
        max_length = maxLengthMarks_;
    else {
        assert( visibility_ == Visibility::MarksAndMatches );

        max_length = qMax( maxLength_, maxLengthMarks_ );
    }

    return max_length;
}

// Implementation of the virtual function.
LineLength LogFilteredData::doGetLineLength( LineNumber lineNum ) const
{
    LineNumber line = findLogDataLine( lineNum );
    return sourceLogData_->getLineLength( line );
}

void LogFilteredData::doSetDisplayEncoding( const char* encoding )
{
    LOG(logDEBUG) << "AbstractLogData::setDisplayEncoding: " << encoding;
}

QTextCodec* LogFilteredData::doGetDisplayEncoding() const
{
    return sourceLogData_->getDisplayEncoding();
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

        const auto next_mark = ( j != marks_.end() ) ? j->lineNumber() : maxValue<LineNumber>();
        const auto next_match = ( i != matching_lines_.cend() ) ? i->lineNumber() : maxValue<LineNumber>();

        LineNumber line;
        FilteredLineType lineType = FilteredLineTypeFlags::None;

        if ( next_mark <= next_match ) {
            // LOG(logDEBUG) << "Add mark at " << next_mark;
            line = next_mark;
            lineType = FilteredLineTypeFlags::Mark;
            ++j;
        }

        if ( next_mark >= next_match ) {
            // LOG(logDEBUG) << "Add match at " << next_match;
            line = next_match;
            lineType |= FilteredLineTypeFlags::Match;
            ++i;
        }

        assert( !! lineType );

        filteredItemsCache_.emplace_back( line, lineType );
    }

    LOG(logDEBUG) << "finished regenerateFilteredItemsCache, size " << filteredItemsCache_.size();
}

void LogFilteredData::insertIntoFilteredItemsCache( FilteredItem&& item )
{
    using std::begin;
    using std::end;

    if ( visibility_ != Visibility::MarksAndMatches ) {
        // this is invalidated and will be regenerated when we need it
        filteredItemsCache_.clear();
        LOG(logDEBUG) << "cache is invalidated";
        return;
    }

    // Search for the corresponding index.
    auto found = std::lower_bound( begin( filteredItemsCache_ ), end( filteredItemsCache_ ), item );
    if ( found == end( filteredItemsCache_ ) || found->lineNumber() > item.lineNumber() ) {
        filteredItemsCache_.emplace( found, std::move( item ) );
    } else {
        assert( found->lineNumber() == item.lineNumber() );
        found->add( item.type() );
    }
}

void LogFilteredData::removeFromFilteredItemsCache( FilteredItem&& item )
{
    using std::begin;
    using std::end;

    if ( visibility_ != Visibility::MarksAndMatches ) {
        // this is invalidated and will be regenerated when we need it
        filteredItemsCache_.clear();
        LOG(logDEBUG) << "cache is invalidated";
        return;
    }

    // Search for the corresponding index.
    auto found = std::equal_range( begin( filteredItemsCache_ ), end( filteredItemsCache_ ), item );
    if( found.first == found.second ) {
        LOG(logERROR) << "Attempt to remove line " << item.lineNumber() << " from filteredItemsCache_ failed, since it was not found";
        return;
    }

    if ( next( found.first ) != found.second ) {
        LOG(logERROR) << "Multiple matches found for line " << item.lineNumber() << " in filteredItemsCache_";
        // FIXME: collapse them?
    }

    if ( !found.first->remove( item.type() ) ) {
        filteredItemsCache_.erase( found.first );
    }
}

void LogFilteredData::removeAllFromFilteredItemsCache( FilteredLineType type )
{
    using std::begin;
    using std::end;

    if ( visibility_ != Visibility::MarksAndMatches ) {
        // this is invalidated and will be regenerated when we need it
        filteredItemsCache_.clear();
        LOG(logDEBUG) << "cache is invalidated";
        return;
    }

    auto erase_begin = std::remove_if( begin( filteredItemsCache_ ), end( filteredItemsCache_ ), [type]( FilteredItem& item ) { return !item.remove( type ); } );
    filteredItemsCache_.erase( erase_begin, end( filteredItemsCache_ ) );
}

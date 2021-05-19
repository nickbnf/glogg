/*
 * Copyright (C) 2009, 2010, 2013, 2014, 2015 Nicolas Bonnefon and other contributors
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

// This file implements LogData, the content of a log file.

#include <iostream>

#include <cassert>

#include <QFileInfo>
#include <QIODevice>
#include <plog/Log.h>
#include <string_view>
#include <utility>
#include <vector>

#include "log.h"

#include "logdata.h"
#include "logfiltereddata.h"

#include "configuration.h"

LogData::LogData()
    : AbstractLogData()
    , indexing_data_( std::make_shared<IndexingData>() )
    , operationQueue_( [ this ] { attached_file_->attachReader(); } )
    , codec_( QTextCodec::codecForName( "ISO-8859-1" ) )
{
    // Initialise the file watcher
    connect( &FileWatcher::getFileWatcher(), &FileWatcher::fileChanged, this,
             &LogData::fileChangedOnDisk, Qt::QueuedConnection );

    auto worker = std::make_unique<LogDataWorker>( indexing_data_ );

    // Forward the update signal
    connect( worker.get(), &LogDataWorker::indexingProgressed, this, &LogData::loadingProgressed );
    connect( worker.get(), &LogDataWorker::indexingFinished, this, &LogData::indexingFinished,
             Qt::QueuedConnection );
    connect( worker.get(), &LogDataWorker::checkFileChangesFinished, this,
             &LogData::checkFileChangesFinished, Qt::QueuedConnection );

    operationQueue_.setWorker( std::move( worker ) );

    const auto& config = Configuration::get();
    keepFileClosed_ = config.keepFileClosed();

    if ( keepFileClosed_ ) {
        LOG_INFO << "Keep file closed option is set";
    }
}

LogData::~LogData()
{
    LOG_DEBUG << "Destroying log data";
    operationQueue_.shutdown();
}

void LogData::attachFile( const QString& fileName )
{
    LOG_DEBUG << "LogData::attachFile " << fileName.toStdString();

    if ( attached_file_ ) {
        // We cannot reattach
        throw CantReattachErr();
    }

    indexingFileName_ = fileName;
    attached_file_.reset( new FileHolder( keepFileClosed_ ) );
    attached_file_->open( indexingFileName_ );

    operationQueue_.enqueueOperation<AttachOperation>( fileName );
}

void LogData::interruptLoading()
{
    operationQueue_.interrupt();
}

qint64 LogData::getFileSize() const
{
    return IndexingData::ConstAccessor{ indexing_data_.get() }.getIndexedSize();
}

QDateTime LogData::getLastModifiedDate() const
{
    return lastModifiedDate_;
}

// Return an initialised LogFilteredData. The search is not started.
std::unique_ptr<LogFilteredData> LogData::getNewFilteredData() const
{
    return std::make_unique<LogFilteredData>( this );
}

void LogData::reload( QTextCodec* forcedEncoding )
{
    operationQueue_.interrupt();

    // Re-open the file, useful in case the file has been moved
    attached_file_->reOpenFile();

    operationQueue_.enqueueOperation<FullReindexOperation>( forcedEncoding );
}

void LogData::fileChangedOnDisk( const QString& filename )
{
    LOG_INFO << "signalFileChanged " << filename << ", indexed file " << indexingFileName_;

    QFileInfo info( indexingFileName_ );
    const auto currentFileId = FileId::getFileId( indexingFileName_ );
    const auto attachedFileId = attached_file_->getFileId();

    const auto indexedHash = IndexingData::ConstAccessor{ indexing_data_.get() }.getHash();

    LOG_INFO << "current indexed fileSize=" << indexedHash.size;
    LOG_INFO << "current indexed hash=" << indexedHash.fullDigest;
    LOG_INFO << "info file_->size()=" << info.size();

    LOG_INFO << "attached_file_->size()=" << attached_file_->size();
    LOG_INFO << "attached_file_id_ index " << attachedFileId.fileIndex;
    LOG_INFO << "currentFileId index " << currentFileId.fileIndex;

    // In absence of any clearer information, we use the following size comparison
    // to determine whether we are following the same file or not (i.e. the file
    // has been moved and the inode we are following is now under a new name, if for
    // instance log has been rotated). We want to follow the name so we have to reopen
    // the file to ensure we are reading the right one.
    // This is a crude heuristic but necessary for notification services that do not
    // give details (e.g. kqueues)

    const bool isFileIdChanged = attachedFileId != currentFileId;

    if ( !isFileIdChanged && filename != indexingFileName_ ) {
        LOG_INFO << "ignore other file update";
        return;
    }

    if ( isFileIdChanged || ( info.size() != attached_file_->size() )
         || ( !attached_file_->isOpen() ) ) {

        LOG_INFO << "Inconsistent size, or file index, the file might have changed, re-opening";

        attached_file_->reOpenFile();
    }

    operationQueue_.enqueueOperation<CheckDataChangesOperation>();
}

void LogData::indexingFinished( LoadingStatus status )
{
    attached_file_->detachReader();

    LOG_INFO << "indexingFinished for: " << indexingFileName_
             << ( status == LoadingStatus::Successful ) << ", found "
             << IndexingData::ConstAccessor{ indexing_data_.get() }.getNbLines() << " lines.";

    if ( status == LoadingStatus::Successful ) {
        FileWatcher::getFileWatcher().addFile( indexingFileName_ );

        // Update the modified date/time if the file exists
        lastModifiedDate_ = QDateTime();
        QFileInfo fileInfo( indexingFileName_ );
        if ( fileInfo.exists() )
            lastModifiedDate_ = fileInfo.lastModified();
    }

    fileChangedOnDisk_ = MonitoredFileStatus::Unchanged;

    LOG_DEBUG << "Sending indexingFinished.";
    emit loadingFinished( status );

    operationQueue_.finishOperationAndStartNext();
}

void LogData::checkFileChangesFinished( MonitoredFileStatus status )
{
    attached_file_->detachReader();

    LOG_INFO << "File " << indexingFileName_ << " status " << static_cast<int>( status );

    if ( fileChangedOnDisk_ != MonitoredFileStatus::Truncated ) {
        switch ( status ) {
        case MonitoredFileStatus::Truncated:
            fileChangedOnDisk_ = MonitoredFileStatus::Truncated;
            operationQueue_.enqueueOperation<FullReindexOperation>();
            break;
        case MonitoredFileStatus::DataAdded:
            fileChangedOnDisk_ = MonitoredFileStatus::DataAdded;
            operationQueue_.enqueueOperation<PartialReindexOperation>();
            break;
        case MonitoredFileStatus::Unchanged:
            fileChangedOnDisk_ = MonitoredFileStatus::Unchanged;
            break;
        }
    }
    else {
        operationQueue_.enqueueOperation<FullReindexOperation>();
    }

    if ( status != MonitoredFileStatus::Unchanged
         || fileChangedOnDisk_ == MonitoredFileStatus::Truncated ) {
        emit fileChanged( fileChangedOnDisk_ );
    }

    operationQueue_.finishOperationAndStartNext();
}

//
// Implementation of virtual functions
//
LinesCount LogData::doGetNbLine() const
{
    return IndexingData::ConstAccessor{ indexing_data_.get() }.getNbLines();
}

LineLength LogData::doGetMaxLength() const
{
    return IndexingData::ConstAccessor{ indexing_data_.get() }.getMaxLength();
}

LineLength LogData::doGetLineLength( LineNumber line ) const
{
    if ( line >= IndexingData::ConstAccessor{ indexing_data_.get() }.getNbLines() ) {
        return 0_length; /* exception? */
    }

    return LineLength( doGetExpandedLineString( line ).length() );
}

void LogData::doSetDisplayEncoding( const char* encoding )
{
    LOG_DEBUG << "AbstractLogData::setDisplayEncoding: " << encoding;
    codec_.setCodec( QTextCodec::codecForName( encoding ) );
    auto needReload = false;
    auto useGuessedCodec = false;

    {
        IndexingData::ConstAccessor scopedAccessor{ indexing_data_.get() };

        const QTextCodec* currentIndexCodec = scopedAccessor.getForcedEncoding();
        if ( !currentIndexCodec ) {
            currentIndexCodec = scopedAccessor.getEncodingGuess();
        }

        if ( codec_.mibEnum() != currentIndexCodec->mibEnum() ) {
            if ( codec_.encodingParameters() != EncodingParameters( currentIndexCodec ) ) {
                needReload = true;
                useGuessedCodec = codec_.mibEnum() == scopedAccessor.getEncodingGuess()->mibEnum();
            }
        }
    }

    if ( needReload ) {
        reload( useGuessedCodec ? nullptr : codec_.codec() );
    }
}

QTextCodec* LogData::doGetDisplayEncoding() const
{
    return codec_.codec();
}

QString LogData::doGetLineString( LineNumber line ) const
{
    const auto lines = doGetLines( line, 1_lcount );
    return lines.empty() ? QString{} : lines.front();
}

QString LogData::doGetExpandedLineString( LineNumber line ) const
{
    return untabify( doGetLineString( line ) );
}

// Note this function is also called from the LogFilteredDataWorker thread, so
// data must be protected because they are changed in the main thread (by
// indexingFinished).
std::vector<QString> LogData::doGetLines( LineNumber first_line, LinesCount number ) const
{
    return getLinesFromFile( first_line, number, []( QString&& lineData ) {
        if ( lineData.endsWith( QChar::CarriageReturn ) ) {
            lineData.chop( 1 );
        }
        return std::move( lineData );
    } );
}

std::vector<QString> LogData::doGetExpandedLines( LineNumber first_line, LinesCount number ) const
{
    return getLinesFromFile( first_line, number,
                             []( QString&& lineData ) { return untabify( lineData ); } );
}

LogData::RawLines LogData::getLinesRaw( LineNumber firstLine, LinesCount number ) const
{
    RawLines rawLines;
    rawLines.startLine = firstLine;
    rawLines.numberOfLines = number;

    try {
        const auto lastLine = firstLine + number - 1_lcount;

        rawLines.endOfLines.reserve( number.get() );

        IndexingData::ConstAccessor scopedAccessor{ indexing_data_.get() };

        if ( lastLine >= scopedAccessor.getNbLines() ) {
            LOG_WARNING << "Lines out of bound asked for";
            return {}; /* exception? */
        }

        ScopedFileHolder<FileHolder> locker( attached_file_.get() );

        const auto firstByte = ( firstLine.get() == 0 )
                                   ? 0
                                   : scopedAccessor.getPosForLine( firstLine - 1_lcount ).get();
        const auto lastByte = scopedAccessor.getPosForLine( lastLine ).get();

        for ( LineNumber line = firstLine; ( line <= lastLine ); ++line ) {
            rawLines.endOfLines.emplace_back( scopedAccessor.getPosForLine( line ).get()
                                              - firstByte );
        }

        const auto bytesToRead = lastByte - firstByte;
        LOG_DEBUG << "will try to read:" << bytesToRead << " bytes";
        rawLines.buffer.resize( static_cast<std::size_t>( bytesToRead ) );

        locker.getFile()->seek( firstByte );
        const auto bytesRead = locker.getFile()->read( rawLines.buffer.data(), bytesToRead );

        if ( bytesRead != bytesToRead ) {
            LOG_DEBUG << "failed to read " << bytesToRead << " bytes, got " << bytesRead;
        }

        LOG_DEBUG << "done reading lines:" << rawLines.buffer.size();
        rawLines.textDecoder = codec_.makeDecoder();
        return rawLines;

    } catch ( const std::bad_alloc& ) {
        LOG_ERROR << "not enough memory";
        rawLines.endOfLines.clear();
        rawLines.buffer.clear();
        rawLines.numberOfLines = 0_lcount;
        return rawLines;
    }
}

std::vector<QString> LogData::getLinesFromFile( LineNumber firstLine, LinesCount number,
                                                QString ( *processLine )( QString&& ) ) const
{
    LOG_DEBUG << "firstLine:" << firstLine << " nb:" << number;

    if ( number.get() == 0 ) {
        return std::vector<QString>();
    }

    std::vector<QString> processedLines;
    try {
        const auto rawLines = getLinesRaw( firstLine, number );
        auto decodedLines = rawLines.decodeLines();

        processedLines.reserve( decodedLines.size() );

        for ( auto&& line : decodedLines ) {
            processedLines.push_back( processLine( std::move( line ) ) );
        }

    } catch ( const std::bad_alloc& e ) {
        LOG_ERROR << "not enough memory " << e.what();
        processedLines.emplace_back( "KLOGG WARNING: not enough memory" );
    }

    while ( processedLines.size() < number.get() ) {
        processedLines.emplace_back( "KLOGG WARNING: failed to read some lines before this one" );
    }

    return processedLines;
}

QTextCodec* LogData::getDetectedEncoding() const
{
    return IndexingData::ConstAccessor{ indexing_data_.get() }.getEncodingGuess();
}

void LogData::doAttachReader() const
{
    attached_file_->attachReader();
}

void LogData::doDetachReader() const
{
    attached_file_->detachReader();
}

std::vector<QString> LogData::RawLines::decodeLines() const
{

    if ( numberOfLines.get() == 0 ) {
        return std::vector<QString>();
    }

    std::vector<QString> decodedLines;
    decodedLines.reserve( numberOfLines.get() );

    const auto lastLine = startLine + numberOfLines - 1_lcount;

    try {
        qint64 lineStart = 0;
        size_t currentLine = 0;
        const auto lineFeedWidth = textDecoder.encodingParams.lineFeedWidth;

        for ( LineNumber line = startLine; ( line <= lastLine ); ++line, ++currentLine ) {
            const auto lineEnd = endOfLines[ currentLine ];

            const auto length = lineEnd - lineStart - lineFeedWidth;

            LOG_DEBUG << "line " << line << ", length " << length;

            if ( length >= std::numeric_limits<LineLength::UnderlyingType>::max() / 2 ) {
                decodedLines.emplace_back( "KLOGG WARNING: this line is too long" );
                break;
            }

            if ( lineStart + length > static_cast<qint64>( buffer.size() ) ) {
                decodedLines.emplace_back( "KLOGG WARNING: file read failed" );
                LOG_WARNING << "not enough data in buffer";
                break;
            }

            decodedLines.push_back( textDecoder.decoder->toUnicode( buffer.data() + lineStart,
                                                                    static_cast<int>( length ) ) );

            lineStart = lineEnd;
        }
    } catch ( const std::bad_alloc& ) {
        LOG_ERROR << "not enough memory";
        decodedLines.emplace_back( "KLOGG WARNING: not enough memory" );
    }

    while ( decodedLines.size() < numberOfLines.get() ) {
        decodedLines.emplace_back( "KLOGG WARNING: failed to read some lines before this one" );
    }

    return decodedLines;
}

std::vector<std::string_view> LogData::RawLines::buildUtf8View() const
{
    std::vector<std::string_view> lines;
    lines.reserve( numberOfLines.get() );

    try {
        const auto utf16Data
            = textDecoder.decoder->toUnicode( buffer.data(), static_cast<int>( buffer.size() ) );

        utf8Data_ = utf16Data.toUtf8();
        std::string_view wholeString{ utf8Data_.data(), static_cast<size_t>( utf8Data_.length() ) };

        auto nextLineFeed = wholeString.find( '\n' );
        while ( nextLineFeed != std::string_view::npos ) {
            lines.push_back( wholeString.substr( 0, nextLineFeed ) );
            wholeString.remove_prefix( nextLineFeed + 1 );
            nextLineFeed = wholeString.find( '\n' );
        }

        if ( !wholeString.empty() ) {
            lines.push_back( wholeString );
        }

    } catch ( const std::exception& e ) {
        LOG_ERROR << "failed to transform lines to utf8 " << e.what();
        const auto lastLineOffset = utf8Data_.size();
        utf8Data_.append( "KLOGG WARNING: not enough memory, try decrease search buffer" );
        while ( lines.size() < numberOfLines.get() ) {
            lines.emplace_back( utf8Data_.data() + lastLineOffset,
                                utf8Data_.size() - lastLineOffset );
        }
    }

    return lines;
}

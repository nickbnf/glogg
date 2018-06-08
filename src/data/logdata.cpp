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

// This file implements LogData, the content of a log file.

#include <iostream>

#include <cassert>

#include <QFileInfo>

#include "log.h"

#include "logdata.h"
#include "logfiltereddata.h"
#if defined(GLOGG_SUPPORTS_INOTIFY) || defined(GLOGG_SUPPORTS_KQUEUE) || defined(WIN32)
#include "platformfilewatcher.h"
#else
#include "qtfilewatcher.h"
#endif

// Implementation of the 'start' functions for each operation

void LogData::AttachOperation::doStart(
        LogDataWorkerThread& workerThread ) const
{
    LOG(logDEBUG) << "Attaching " << filename_.toStdString();
    workerThread.attachFile( filename_ );
    workerThread.indexAll();
}

void LogData::FullIndexOperation::doStart(
        LogDataWorkerThread& workerThread ) const
{
    LOG(logDEBUG) << "Reindexing (full)";
    workerThread.indexAll();
}

void LogData::PartialIndexOperation::doStart(
        LogDataWorkerThread& workerThread ) const
{
    LOG(logDEBUG) << "Reindexing (partial)";
    workerThread.indexAdditionalLines();
}


// Constructs an empty log file.
// It must be displayed without error.
LogData::LogData() : AbstractLogData(), indexing_data_(),
    fileMutex_(), workerThread_( indexing_data_ )
{
    // Start with an "empty" log
    attached_file_ = nullptr;
    currentOperation_ = nullptr;
    nextOperation_    = nullptr;

    codec_ = QTextCodec::codecForName( "ISO-8859-1" );

#if defined(GLOGG_SUPPORTS_INOTIFY) || defined(GLOGG_SUPPORTS_KQUEUE) || defined(WIN32)
    fileWatcher_ = std::make_shared<PlatformFileWatcher>();
#else
    fileWatcher_ = std::make_shared<QtFileWatcher>();
#endif

    // Initialise the file watcher
    connect( fileWatcher_.get(), SIGNAL( fileChanged( const QString& ) ),
            this, SLOT( fileChangedOnDisk() ) );
    // Forward the update signal
    connect( &workerThread_, SIGNAL( indexingProgressed( int ) ),
            this, SIGNAL( loadingProgressed( int ) ) );
    connect( &workerThread_, SIGNAL( indexingFinished( LoadingStatus ) ),
            this, SLOT( indexingFinished( LoadingStatus ) ) );

    // Starts the worker thread
    workerThread_.start();
}

LogData::~LogData()
{
    // Remove the current file from the watch list
    if ( attached_file_ )
        fileWatcher_->removeFile( attached_file_->fileName() );

    // FIXME
    // workerThread_.stop();
}

//
// Public functions
//

void LogData::attachFile( const QString& fileName )
{
    LOG(logDEBUG) << "LogData::attachFile " << fileName.toStdString();

    if ( attached_file_ ) {
        // We cannot reattach
        throw CantReattachErr();
    }

    attached_file_.reset( new QFile( fileName ) );
    attached_file_->open( QIODevice::ReadOnly );

    std::shared_ptr<const LogDataOperation> operation( new AttachOperation( fileName ) );
    enqueueOperation( std::move( operation ) );
}

void LogData::interruptLoading()
{
    workerThread_.interrupt();
}

qint64 LogData::getFileSize() const
{
    return indexing_data_.getSize();
}

QDateTime LogData::getLastModifiedDate() const
{
    return lastModifiedDate_;
}

// Return an initialised LogFilteredData. The search is not started.
LogFilteredData* LogData::getNewFilteredData() const
{
    LogFilteredData* newFilteredData = new LogFilteredData( this );

    return newFilteredData;
}

void LogData::reload()
{
    workerThread_.interrupt();

    enqueueOperation( std::make_shared<FullIndexOperation>() );
}

void LogData::setPollingInterval( uint32_t interval_ms )
{
    fileWatcher_->setPollingInterval( interval_ms );
}

//
// Private functions
//

// Add an operation to the queue and perform it immediately if
// there is none ongoing.
void LogData::enqueueOperation( std::shared_ptr<const LogDataOperation> new_operation )
{
    if ( currentOperation_ == nullptr )
    {
        // We do it immediately
        currentOperation_ =  new_operation;
        startOperation();
    }
    else
    {
        // An operation is in progress...
        // ... we schedule the attach op for later
        nextOperation_ = new_operation;
    }
}

// Performs the current operation asynchronously, a indexingFinished
// signal will be received when it's finished.
void LogData::startOperation()
{
    if ( currentOperation_ )
    {
        LOG(logDEBUG) << "startOperation found something to do.";

        // Let the operation do its stuff
        currentOperation_->start( workerThread_ );
    }
}

//
// Slots
//

void LogData::fileChangedOnDisk()
{
    LOG(logDEBUG) << "signalFileChanged";

    const QString name = attached_file_->fileName();
    QFileInfo info( name );

    // Need to open the file in case it was absent
    attached_file_->open( QIODevice::ReadOnly );

    std::shared_ptr<LogDataOperation> newOperation;

    qint64 file_size = indexing_data_.getSize();
    LOG(logDEBUG) << "current fileSize=" << file_size;
    LOG(logDEBUG) << "info file_->size()=" << info.size();
    if ( info.size() < file_size ) {
        fileChangedOnDisk_ = Truncated;
        LOG(logINFO) << "File truncated";
        newOperation = std::make_shared<FullIndexOperation>();
    }
    else if ( info.size() == file_size ) {
        LOG(logINFO) << "No change in file";
    }
    else if ( fileChangedOnDisk_ != DataAdded ) {
        fileChangedOnDisk_ = DataAdded;
        LOG(logINFO) << "New data on disk";
        newOperation = std::make_shared<PartialIndexOperation>();
    }

    if ( newOperation ) {
        enqueueOperation( newOperation );
        lastModifiedDate_ = info.lastModified();

        emit fileChanged( fileChangedOnDisk_ );
    }
}

void LogData::indexingFinished( LoadingStatus status )
{
    LOG(logDEBUG) << "indexingFinished: " <<
        ( status == LoadingStatus::Successful ) <<
        ", found " << indexing_data_.getNbLines() << " lines.";

    if ( status == LoadingStatus::Successful ) {
        // Start watching we watch the file for updates
        fileChangedOnDisk_ = Unchanged;
        fileWatcher_->addFile( attached_file_->fileName() );

        // Update the modified date/time if the file exists
        lastModifiedDate_ = QDateTime();
        QFileInfo fileInfo( *attached_file_ );
        if ( fileInfo.exists() )
            lastModifiedDate_ = fileInfo.lastModified();
    }

    // FIXME be cleverer here as a notification might have arrived whilst we
    // were indexing.
    fileChangedOnDisk_ = Unchanged;

    LOG(logDEBUG) << "Sending indexingFinished.";
    emit loadingFinished( status );

    // So now the operation is done, let's see if there is something
    // else to do, in which case, do it!
    assert( currentOperation_ );

    currentOperation_ = std::move( nextOperation_ );
    nextOperation_.reset();

    if ( currentOperation_ ) {
        LOG(logDEBUG) << "indexingFinished is performing the next operation";
        startOperation();
    }
}

//
// Implementation of virtual functions
//
qint64 LogData::doGetNbLine() const
{
    return indexing_data_.getNbLines();
}

int LogData::doGetMaxLength() const
{
    return indexing_data_.getMaxLength();
}

int LogData::doGetLineLength( qint64 line ) const
{
    if ( line >= indexing_data_.getNbLines() ) { return 0; /* exception? */ }

    int length = doGetExpandedLineString( line ).length();

    return length;
}

void LogData::doSetDisplayEncoding( Encoding encoding )
{
    LOG(logDEBUG) << "AbstractLogData::setDisplayEncoding: " << static_cast<int>( encoding );

    static const char* latin1_encoding = "iso-8859-1";
    static const char* utf8_encoding   = "utf-8";
    static const char* utf16le_encoding   = "utf-16le";
    static const char* utf16be_encoding   = "utf-16be";
    static const char* cp1251_encoding   = "CP1251";
    static const char* cp1252_encoding   = "CP1252";

    const char* qt_encoding = latin1_encoding;

    // Default to 0, for 8bit encodings
    int before_cr = 0;
    int after_cr  = 0;

    switch ( encoding ) {
        case Encoding::ENCODING_UTF8:
            qt_encoding = utf8_encoding;
            break;
        case Encoding::ENCODING_UTF16LE:
            qt_encoding = utf16le_encoding;
            before_cr = 0;
            after_cr  = 1;
            break;
        case Encoding::ENCODING_UTF16BE:
            qt_encoding = utf16be_encoding;
            before_cr = 1;
            after_cr  = 0;
            break;
        case Encoding::ENCODING_CP1251:
            qt_encoding = cp1251_encoding;
            break;
        case Encoding::ENCODING_CP1252:
            qt_encoding = cp1252_encoding;
            break;
        case Encoding::ENCODING_ISO_8859_1:
            qt_encoding = latin1_encoding;
            break;
        default:
            LOG( logERROR ) << "Unknown encoding set!";
            assert( false );
            break;
    }

    doSetMultibyteEncodingOffsets( before_cr, after_cr );
    codec_ = QTextCodec::codecForName( qt_encoding );
}

void LogData::doSetMultibyteEncodingOffsets( int before_cr, int after_cr )
{
    before_cr_offset_ = before_cr;
    after_cr_offset_ = after_cr;
}

QString LogData::doGetLineString( qint64 line ) const
{
    if ( line >= indexing_data_.getNbLines() ) { return 0; /* exception? */ }

    fileMutex_.lock();

    // end_byte is non-inclusive.(is not read)
    const qint64 first_byte = (line == 0) ?
        0 : ( indexing_data_.getPosForLine( line-1 ) + after_cr_offset_ );
    const qint64 end_byte  = endOfLinePosition( line );

    attached_file_->seek( first_byte );

    QString string = codec_->toUnicode( attached_file_->read( end_byte - first_byte ) );

    fileMutex_.unlock();

    return string;
}

QString LogData::doGetExpandedLineString( qint64 line ) const
{
    if ( line >= indexing_data_.getNbLines() ) { return 0; /* exception? */ }

    fileMutex_.lock();

    // end_byte is non-inclusive.(is not read) We also exclude the final \r.
    const qint64 first_byte = (line == 0) ?
        0 : ( indexing_data_.getPosForLine( line-1 ) + after_cr_offset_ );
    const qint64 end_byte  = endOfLinePosition( line );

    attached_file_->seek( first_byte );

    // LOG(logDEBUG) << "LogData::doGetExpandedLineString first_byte:" << first_byte << " end_byte:" << end_byte;
    QByteArray rawString = attached_file_->read( end_byte - first_byte );

    fileMutex_.unlock();

    QString string = untabify( codec_->toUnicode( rawString ) );

    // LOG(logDEBUG) << "doGetExpandedLineString Line is: " << string.toStdString();

    return string;
}

// Note this function is also called from the LogFilteredDataWorker thread, so
// data must be protected because they are changed in the main thread (by
// indexingFinished).
QStringList LogData::doGetLines( qint64 first_line, int number ) const
{
    QStringList list;
    const qint64 last_line = first_line + number - 1;

    // LOG(logDEBUG) << "LogData::doGetLines first_line:" << first_line << " nb:" << number;

    if ( number == 0 ) {
        return QStringList();
    }

    if ( last_line >= indexing_data_.getNbLines() ) {
        LOG(logWARNING) << "LogData::doGetLines Lines out of bound asked for";
        return QStringList(); /* exception? */
    }

    fileMutex_.lock();

    const qint64 first_byte = (first_line == 0) ?
        0 : ( indexing_data_.getPosForLine( first_line-1 ) + after_cr_offset_ );
    const qint64 end_byte  = endOfLinePosition( last_line );
    // LOG(logDEBUG) << "LogData::doGetLines first_byte:" << first_byte << " end_byte:" << end_byte;
    attached_file_->seek( first_byte );
    QByteArray blob = attached_file_->read( end_byte - first_byte );

    fileMutex_.unlock();

    qint64 beginning = 0;
    qint64 end = 0;
    for ( qint64 line = first_line; (line <= last_line); line++ ) {
        end = endOfLinePosition( line ) - first_byte;
        // LOG(logDEBUG) << "Getting line " << line << " beginning " << beginning << " end " << end;
        QByteArray this_line = blob.mid( beginning, end - beginning );
        // LOG(logDEBUG) << "Line is: " << QString( this_line ).toStdString();
        list.append( codec_->toUnicode( this_line ) );
        beginning = beginningOfNextLine( end );
    }

    return list;
}

QStringList LogData::doGetExpandedLines( qint64 first_line, int number ) const
{
    QStringList list;
    const qint64 last_line = first_line + number - 1;

    if ( number == 0 ) {
        return QStringList();
    }

    if ( last_line >= indexing_data_.getNbLines() ) {
        LOG(logWARNING) << "LogData::doGetExpandedLines Lines out of bound asked for";
        return QStringList(); /* exception? */
    }

    fileMutex_.lock();

    // end_byte is non-inclusive.(is not read)
    const qint64 first_byte = (first_line == 0) ?
        0 : ( indexing_data_.getPosForLine( first_line-1 ) + after_cr_offset_ );
    const qint64 end_byte  = endOfLinePosition( last_line );
    LOG(logDEBUG) << "LogData::doGetExpandedLines first_byte:" << first_byte << " end_byte:" << end_byte;

    attached_file_->seek( first_byte );
    QByteArray blob = attached_file_->read( end_byte - first_byte );

    fileMutex_.unlock();

    qint64 beginning = 0;
    qint64 end = 0;
    for ( qint64 line = first_line; (line <= last_line); line++ ) {
        // end is non-inclusive
        // LOG(logDEBUG) << "EoL " << line << ": " << indexing_data_.getPosForLine( line );
        end = endOfLinePosition( line ) - first_byte;
        // LOG(logDEBUG) << "Getting line " << line << " beginning " << beginning << " end " << end;
        QByteArray this_line = blob.mid( beginning, end - beginning );
        QString conv_line = codec_->toUnicode( this_line );
        // LOG(logDEBUG) << "Line is: " << conv_line.toStdString();
        list.append( untabify( conv_line ) );
        beginning = beginningOfNextLine( end );
    }

    return list;
}

EncodingSpeculator::Encoding LogData::getDetectedEncoding() const
{
    return indexing_data_.getEncodingGuess();
}

// Given a line number, returns the position (offset in file) of
// the byte immediately past its end.
// e.g. in utf-16: T e s t \n2 n d l i n e \n
//                 --------------------------
//                           ^
//                   endOfLinePosition( 0 )
qint64 LogData::endOfLinePosition( qint64 line ) const
{
    return indexing_data_.getPosForLine( line ) - 1 - before_cr_offset_;
}

// Given the position (offset in file) of the end of a line, returns
// the position of the beginning of the following, taking into account
// encoding and newline signalling.
qint64 LogData::beginningOfNextLine( qint64 end_pos ) const
{
    return end_pos + 1 + before_cr_offset_ + after_cr_offset_;
}

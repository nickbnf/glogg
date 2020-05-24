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

#include "log.h"

#include "logdata.h"
#include "logfiltereddata.h"

#include "configuration.h"

// Implementation of the 'start' functions for each operation

void LogData::AttachOperation::doStart( LogDataWorker& workerThread ) const
{
    LOG( logINFO ) << "Attaching " << filename_.toStdString();
    workerThread.attachFile( filename_ );
    workerThread.indexAll();
}

void LogData::FullIndexOperation::doStart( LogDataWorker& workerThread ) const
{
    LOG( logINFO ) << "Reindexing (full)";
    workerThread.indexAll( forcedEncoding_ );
}

void LogData::PartialIndexOperation::doStart( LogDataWorker& workerThread ) const
{
    LOG( logINFO ) << "Reindexing (partial)";
    workerThread.indexAdditionalLines();
}

void LogData::CheckFileChangesOperation::doStart( LogDataWorker& workerThread ) const
{
    LOG( logINFO ) << "Checking file changes";
    workerThread.checkFileChanges();
}

// Constructs an empty log file.
// It must be displayed without error.
LogData::LogData()
    : AbstractLogData()
    , indexing_data_()
    , workerThread_( indexing_data_ )
{
    // Start with an "empty" log
    attached_file_ = nullptr;
    currentOperation_ = nullptr;
    nextOperation_ = nullptr;

    codec_ = QTextCodec::codecForName( "ISO-8859-1" );

    // Initialise the file watcher
    connect( &FileWatcher::getFileWatcher(), &FileWatcher::fileChanged, this,
             &LogData::fileChangedOnDisk, Qt::QueuedConnection );

    // Forward the update signal
    connect( &workerThread_, &LogDataWorker::indexingProgressed, this,
             &LogData::loadingProgressed );
    connect( &workerThread_, &LogDataWorker::indexingFinished, this, &LogData::indexingFinished );
    connect( &workerThread_, &LogDataWorker::checkFileChangesFinished, this,
             &LogData::checkFileChangesFinished );

    const auto& config = Configuration::get();
    keepFileClosed_ = config.keepFileClosed();

    if ( keepFileClosed_ ) {
        LOG( logINFO ) << "Keep file closed option is set";
    }
}

LogData::~LogData() {}

//
// Public functions
//

void LogData::attachFile( const QString& fileName )
{
    LOG( logDEBUG ) << "LogData::attachFile " << fileName.toStdString();

    if ( attached_file_ ) {
        // We cannot reattach
        throw CantReattachErr();
    }

    indexingFileName_ = fileName;
    attached_file_.reset( new FileHolder( keepFileClosed_ ) );
    attached_file_->open( indexingFileName_ );

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
std::unique_ptr<LogFilteredData> LogData::getNewFilteredData() const
{
    return std::make_unique<LogFilteredData>( this );
}

void LogData::reload( QTextCodec* forcedEncoding )
{
    workerThread_.interrupt();

    // Re-open the file, useful in case the file has been moved
    attached_file_->reOpenFile();

    enqueueOperation( std::make_shared<FullIndexOperation>( forcedEncoding ) );
}

//
// Private functions
//

// Add an operation to the queue and perform it immediately if
// there is none ongoing.
void LogData::enqueueOperation( std::shared_ptr<const LogDataOperation> new_operation )
{
    if ( currentOperation_ == nullptr ) {
        // We do it immediately
        currentOperation_ = new_operation;
        startOperation();
    }
    else {
        // An operation is in progress...
        // ... we schedule the attach op for later
        nextOperation_ = new_operation;
    }
}

// Performs the current operation asynchronously, a indexingFinished
// signal will be received when it's finished.
void LogData::startOperation()
{
    attached_file_->attachReader();

    if ( currentOperation_ ) {
        LOG( logDEBUG ) << "startOperation found something to do.";

        // Let the operation do its stuff
        currentOperation_->start( workerThread_ );
    }
}

//
// Slots
//

void LogData::fileChangedOnDisk( const QString& filename )
{
    LOG( logINFO ) << "signalFileChanged " << filename << ", indexed file " << indexingFileName_;

    QFileInfo info( indexingFileName_ );
    const auto currentFileId = FileId::getFileId( indexingFileName_ );
    const auto attachedFileId = attached_file_->getFileId();

    const auto indexedHash = indexing_data_.getHash();

    LOG( logINFO ) << "current indexed fileSize=" << indexedHash.size;
    LOG( logINFO ) << "current indexed hash=" << indexedHash.digest;
    LOG( logINFO ) << "info file_->size()=" << info.size();

    LOG( logINFO ) << "attached_file_->size()=" << attached_file_->size();
    LOG( logINFO ) << "attached_file_id_ index " << attachedFileId.fileIndex;
    LOG( logINFO ) << "currentFileId index " << currentFileId.fileIndex;

    // In absence of any clearer information, we use the following size comparison
    // to determine whether we are following the same file or not (i.e. the file
    // has been moved and the inode we are following is now under a new name, if for
    // instance log has been rotated). We want to follow the name so we have to reopen
    // the file to ensure we are reading the right one.
    // This is a crude heuristic but necessary for notification services that do not
    // give details (e.g. kqueues)

    const bool isFileIdChanged = attachedFileId != currentFileId;

    if ( !isFileIdChanged && filename != indexingFileName_ ) {
        LOG( logINFO ) << "ignore other file update";
        return;
    }

    if ( isFileIdChanged || ( info.size() != attached_file_->size() )
         || ( !attached_file_->isOpen() ) ) {

        LOG( logINFO )
            << "Inconsistent size, or file index, the file might have changed, re-opening";

        attached_file_->reOpenFile();
    }

    enqueueOperation( std::make_shared<CheckFileChangesOperation>() );
}

void LogData::indexingFinished( LoadingStatus status )
{
    attached_file_->detachReader();

    LOG( logDEBUG ) << "indexingFinished for: " << indexingFileName_
                    << ( status == LoadingStatus::Successful ) << ", found "
                    << indexing_data_.getNbLines() << " lines.";

    if ( status == LoadingStatus::Successful ) {
        FileWatcher::getFileWatcher().addFile( indexingFileName_ );

        // Update the modified date/time if the file exists
        lastModifiedDate_ = QDateTime();
        QFileInfo fileInfo( indexingFileName_ );
        if ( fileInfo.exists() )
            lastModifiedDate_ = fileInfo.lastModified();
    }

    fileChangedOnDisk_ = MonitoredFileStatus::Unchanged;

    LOG( logDEBUG ) << "Sending indexingFinished.";
    emit loadingFinished( status );

    // So now the operation is done, let's see if there is something
    // else to do, in which case, do it!
    assert( currentOperation_ );

    currentOperation_ = std::move( nextOperation_ );
    nextOperation_.reset();

    if ( currentOperation_ ) {
        LOG( logDEBUG ) << "indexingFinished is performing the next operation";
        startOperation();
    }
}

void LogData::checkFileChangesFinished( MonitoredFileStatus status )
{
    attached_file_->detachReader();

    LOG( logINFO ) << "File " << indexingFileName_ << " status " << static_cast<int>( status );

    std::function<std::shared_ptr<LogDataOperation>()> newOperation;
    if ( fileChangedOnDisk_ != MonitoredFileStatus::Truncated ) {
        switch ( status ) {
        case MonitoredFileStatus::Truncated:
            fileChangedOnDisk_ = MonitoredFileStatus::Truncated;
            newOperation = std::make_shared<FullIndexOperation>;
            break;
        case MonitoredFileStatus::DataAdded:
            fileChangedOnDisk_ = MonitoredFileStatus::DataAdded;
            newOperation = std::make_shared<PartialIndexOperation>;
            break;
        case MonitoredFileStatus::Unchanged:
            fileChangedOnDisk_ = MonitoredFileStatus::Unchanged;
            break;
        }
    }
    else {
        newOperation = std::make_shared<FullIndexOperation>;
    }

    if ( newOperation ) {
        emit fileChanged( fileChangedOnDisk_ );

        enqueueOperation( newOperation() );
    }

    currentOperation_ = std::move( nextOperation_ );
    nextOperation_.reset();

    if ( currentOperation_ ) {
        LOG( logDEBUG ) << "checkFileChangesFinished is performing the next operation";
        startOperation();
    }
}

//
// Implementation of virtual functions
//
LinesCount LogData::doGetNbLine() const
{
    return indexing_data_.getNbLines();
}

LineLength LogData::doGetMaxLength() const
{
    return indexing_data_.getMaxLength();
}

LineLength LogData::doGetLineLength( LineNumber line ) const
{
    if ( line >= indexing_data_.getNbLines() ) {
        return 0_length; /* exception? */
    }

    return LineLength( doGetExpandedLineString( line ).length() );
}

void LogData::doSetDisplayEncoding( const char* encoding )
{
    LOG( logDEBUG ) << "AbstractLogData::setDisplayEncoding: " << encoding;
    codec_ = QTextCodec::codecForName( encoding );

    const QTextCodec* currentIndexCodec = indexing_data_.getForcedEncoding();
    if ( !currentIndexCodec )
        currentIndexCodec = indexing_data_.getEncodingGuess();

    if ( codec_->mibEnum() != currentIndexCodec->mibEnum() ) {
        if ( EncodingParameters( codec_ ) != EncodingParameters( currentIndexCodec ) ) {
            bool isGuessedCodec = codec_->mibEnum() == indexing_data_.getEncodingGuess()->mibEnum();
            reload( isGuessedCodec ? nullptr : codec_ );
        }
    }
}

QTextCodec* LogData::doGetDisplayEncoding() const
{
    return codec_;
}

QString LogData::doGetLineString( LineNumber line ) const
{
    if ( line >= indexing_data_.getNbLines() ) {
        return ""; /* exception? */
    }

    QByteArray rawString;

    {
        ScopedFileHolder<FileHolder> locker( attached_file_.get() );

        locker.getFile()->seek(
            ( line.get() == 0 ) ? 0 : indexing_data_.getPosForLine( line - 1_lcount ).get() );
        rawString = locker.getFile()->readLine();
    }

    auto string = codec_->toUnicode( rawString );
    string.chop( 1 );
    if ( string.endsWith( QChar::CarriageReturn ) ) {
        string.chop( 1 );
    }

    return string;
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
    const auto last_line = first_line + number - 1_lcount;

    // LOG(logDEBUG) << "LogData::doGetLines first_line:" << first_line << " nb:" << number;

    if ( number.get() == 0 ) {
        return std::vector<QString>();
    }

    if ( last_line >= indexing_data_.getNbLines() ) {
        LOG( logWARNING ) << "LogData::doGetLines Lines out of bound asked for";
        return std::vector<QString>(); /* exception? */
    }

    const auto first_byte = ( first_line.get() == 0 )
                                ? 0
                                : indexing_data_.getPosForLine( first_line - 1_lcount ).get();
    const auto last_byte = indexing_data_.getPosForLine( last_line ).get();

    QByteArray buffer;

    {
        ScopedFileHolder<FileHolder> locker( attached_file_.get() );

        locker.getFile()->seek( first_byte );
        buffer = locker.getFile()->read( last_byte - first_byte );
    }

    std::vector<QString> list;
    list.reserve( number.get() );

    qint64 beginning = 0;
    qint64 end = 0;
    std::unique_ptr<QTextDecoder> decoder{ codec_->makeDecoder() };

    EncodingParameters encodingParams{ codec_ };

    for ( LineNumber line = first_line; ( line <= last_line ); ++line ) {
        end = indexing_data_.getPosForLine( line ).get() - first_byte;
        auto lineData = decoder->toUnicode( buffer.data() + beginning,
                                            static_cast<LineLength::UnderlyingType>(
                                                end - beginning - encodingParams.lineFeedWidth ) );

        if ( lineData.endsWith( QChar::CarriageReturn ) ) {
            lineData.chop( 1 );
        }

        list.emplace_back( std::move( lineData ) );
        beginning = end;
    }

    return list;
}

std::vector<QString> LogData::doGetExpandedLines( LineNumber first_line, LinesCount number ) const
{
    const auto last_line = first_line + number - 1_lcount;

    if ( number.get() == 0 ) {
        return std::vector<QString>();
    }

    if ( last_line >= indexing_data_.getNbLines() ) {
        LOG( logWARNING ) << "LogData::doGetExpandedLines Lines out of bound asked for";
        return std::vector<QString>(); /* exception? */
    }

    const auto first_byte = ( first_line.get() == 0 )
                                ? 0
                                : indexing_data_.getPosForLine( first_line - 1_lcount ).get();
    const auto last_byte = indexing_data_.getPosForLine( last_line ).get();

    QByteArray buffer;

    {
        ScopedFileHolder<FileHolder> locker( attached_file_.get() );

        locker.getFile()->seek( first_byte );
        buffer = locker.getFile()->read( last_byte - first_byte );
    }

    std::vector<QString> list;
    list.reserve( number.get() );

    qint64 beginning{};
    qint64 end{};
    std::unique_ptr<QTextDecoder> decoder{ codec_->makeDecoder() };

    EncodingParameters encodingParams{ codec_ };

    for ( auto line = first_line; ( line <= last_line ); ++line ) {
        end = indexing_data_.getPosForLine( line ).get() - first_byte;
        list.emplace_back( untabify( decoder->toUnicode(
            buffer.data() + beginning, static_cast<LineLength::UnderlyingType>(
                                           end - beginning - encodingParams.lineFeedWidth ) ) ) );
        beginning = end;
    }

    return list;
}

QTextCodec* LogData::getDetectedEncoding() const
{
    return indexing_data_.getEncodingGuess();
}

void LogData::doAttachReader() const
{
    attached_file_->attachReader();
}

void LogData::doDetachReader() const
{
    attached_file_->detachReader();
}

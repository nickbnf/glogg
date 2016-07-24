/*
 * Copyright (C) 2009, 2010, 2014, 2015 Nicolas Bonnefon and other contributors
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

#include <QFile>
#include <QTextStream>

#include "log.h"

#include "logdata.h"
#include "logdataworkerthread.h"

EncodingParameters::EncodingParameters(const QTextCodec *codec)
{
    static const QChar lineFeed(QChar::LineFeed);
    QTextCodec::ConverterState convertState(QTextCodec::IgnoreHeader);
    QByteArray encodedLineFeed = codec->fromUnicode(&lineFeed, 1, &convertState);

    lineFeedWidth = encodedLineFeed.length();
    lineFeedIndex = encodedLineFeed[0] == '\n' ? 0 : (encodedLineFeed.length() - 1);
}

// Size of the chunk to read (5 MiB)
const int IndexOperation::sizeChunk = 1*1024*1024;

qint64 IndexingData::getSize() const
{
    QMutexLocker locker( &dataMutex_ );

    return indexedSize_;
}

int IndexingData::getMaxLength() const
{
    QMutexLocker locker( &dataMutex_ );

    return maxLength_;
}

LineNumber IndexingData::getNbLines() const
{
    QMutexLocker locker( &dataMutex_ );

    return linePosition_.size();
}

qint64 IndexingData::getPosForLine( LineNumber line ) const
{
    QMutexLocker locker( &dataMutex_ );

    return linePosition_.at( line );
}

QTextCodec *IndexingData::getEncodingGuess() const
{
    QMutexLocker locker( &dataMutex_ );
    return encodingGuess_;
}

void IndexingData::forceEncoding(QTextCodec *codec)
{
     QMutexLocker locker( &dataMutex_ );
     encodingForced_ = codec;
}

QTextCodec* IndexingData::getForcedEncoding() const
{
     QMutexLocker locker( &dataMutex_ );
     return encodingForced_;
}

void IndexingData::addAll(qint64 size, int length,
        const FastLinePositionArray& linePosition,
        QTextCodec *encoding )

{
    QMutexLocker locker( &dataMutex_ );

    indexedSize_  += size;
    maxLength_     = qMax( maxLength_, length );
    linePosition_.append_list( linePosition );

    encodingGuess_      = encoding;
}

void IndexingData::clear()
{
    maxLength_   = 0;
    indexedSize_ = 0;
    linePosition_ = LinePositionArray();
    encodingGuess_    = QTextCodec::codecForLocale();
    encodingForced_ = nullptr;
}

LogDataWorkerThread::LogDataWorkerThread( IndexingData* indexing_data )
    : QThread(), mutex_(), operationRequestedCond_(),
    nothingToDoCond_(), fileName_(), indexing_data_( indexing_data )
{
    operationRequested_ = nullptr;
}

LogDataWorkerThread::~LogDataWorkerThread()
{
    {
        QMutexLocker locker( &mutex_ );
        terminate_.set();
        operationRequestedCond_.wakeAll();
    }
    wait();
}

void LogDataWorkerThread::attachFile( const QString& fileName )
{
    QMutexLocker locker( &mutex_ );  // to protect fileName_

    fileName_ = fileName;
}

void LogDataWorkerThread::indexAll(QTextCodec* forcedEncoding)
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "FullIndex requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_.clear();
    operationRequested_ = new FullIndexOperation( fileName_,
            indexing_data_, &interruptRequested_, forcedEncoding );
    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::indexAdditionalLines( qint64 position )
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "AddLines requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_.clear();
    operationRequested_ = new PartialIndexOperation( fileName_,
            indexing_data_, &interruptRequested_, position );
    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::interrupt()
{
    LOG(logDEBUG) << "Load interrupt requested";
    interruptRequested_.set();
}

// This is the thread's main loop
void LogDataWorkerThread::run()
{
    QMutexLocker locker( &mutex_ );

    forever {
        while ( !terminate_ && (operationRequested_ == NULL) )
            operationRequestedCond_.wait( &mutex_ );
        LOG(logDEBUG) << "Worker thread signaled";

        // Look at what needs to be done
        if ( terminate_ )
            return;      // We must die

        if ( operationRequested_ ) {
            connect( operationRequested_, SIGNAL( indexingProgressed( int ) ),
                    this, SIGNAL( indexingProgressed( int ) ) );

            // Run the operation
            try {
                if ( operationRequested_->start() ) {
                    LOG(logDEBUG) << "... finished copy in workerThread.";
                    emit indexingFinished( LoadingStatus::Successful );
                }
                else {
                    emit indexingFinished( LoadingStatus::Interrupted );
                }
            }
            catch ( std::bad_alloc& ba ) {
                LOG(logERROR) << "Out of memory whilst indexing!";
                emit indexingFinished( LoadingStatus::NoMemory );
            }

            delete operationRequested_;
            operationRequested_ = NULL;
            nothingToDoCond_.wakeAll();
        }
    }
}

//
// Operations implementation
//

IndexOperation::IndexOperation( const QString& fileName,
        IndexingData* indexingData, AtomicFlag* interruptRequest)
    : fileName_( fileName )
{
    interruptRequest_ = interruptRequest;
    indexing_data_ = indexingData;
}

PartialIndexOperation::PartialIndexOperation( const QString& fileName,
        IndexingData* indexingData, AtomicFlag* interruptRequest, qint64 position )
    : IndexOperation( fileName, indexingData, interruptRequest )
{
    initialPosition_ = position;
}

void IndexOperation::doIndex(IndexingData* indexing_data, qint64 initialPosition )
{
    qint64 pos = initialPosition; // Absolute position of the start of current line
    qint64 end = 0;               // Absolute position of the end of current line
    int additional_spaces = 0;    // Additional spaces due to tabs

    QTextCodec* fileTextCodec = nullptr;
    QTextCodec* encodingGuess = nullptr;
    EncodingParameters encodingParams;

    QFile file( fileName_ );

    if ( file.open( QIODevice::ReadOnly ) ) {
        // Count the number of lines and max length
        // (read big chunks to speed up reading from disk)
        file.seek( pos );
        while ( !file.atEnd() ) {
            FastLinePositionArray line_positions;
            int max_length = 0;

            if ( *interruptRequest_ )
                break;

            // Read a chunk of 5MB
            const qint64 block_beginning = file.pos();
            const QByteArray block = file.read( sizeChunk );

            if (!fileTextCodec) {
                fileTextCodec = indexing_data->getForcedEncoding();
                if (!fileTextCodec) fileTextCodec = QTextCodec::codecForUtfText(block);
                encodingParams = EncodingParameters(fileTextCodec);
                LOG(logWARNING) << "Encoding " << fileTextCodec->name().data() <<", Char width " << encodingParams.lineFeedWidth;
            }

            if (!encodingGuess) {
                encodingGuess = QTextCodec::codecForUtfText(block);
            }

            // Count the number of lines in each chunk
            qint64 pos_within_block = 0;
            while ( pos_within_block != -1 ) {
                pos_within_block = qMax( pos - block_beginning, 0LL);

                // Looking for the next \n, expanding tabs in the process
                do {
                    if ( pos_within_block < block.length() ) {
                        const char c =  block.at(pos_within_block + encodingParams.lineFeedIndex);

                        if ( c == '\n')
                            break;
                        else if ( c == '\t')
                            additional_spaces += AbstractLogData::tabStop -
                                ( ( ( block_beginning - pos ) + pos_within_block
                                    + additional_spaces ) % AbstractLogData::tabStop ) - 1;

                        pos_within_block += encodingParams.lineFeedWidth;
                    }
                    else {
                        pos_within_block = -1;
                    }
                } while ( pos_within_block != -1 );

                // When a end of line has been found...
                if ( pos_within_block != -1 ) {
                    end = pos_within_block + block_beginning;
                    const int length = end-pos + additional_spaces;
                    if ( length > max_length )
                        max_length = length;

                    pos = end + encodingParams.lineFeedWidth;
                    additional_spaces = 0;
                    line_positions.append( pos );
                }
            }

            // Update the shared data
            indexing_data->addAll( block.length(), max_length, line_positions,
                   encodingGuess );

            // Update the caller for progress indication
            int progress = ( file.size() > 0 ) ? pos*100 / file.size() : 100;
            emit indexingProgressed( progress );
        }

        // Check if there is a non LF terminated line at the end of the file
        qint64 file_size = file.size();
        if ( !*interruptRequest_ && file_size > pos ) {
            LOG( logWARNING ) <<
                "Non LF terminated file, adding a fake end of line";

            FastLinePositionArray line_position;
            line_position.append( file_size + 1 );
            line_position.setFakeFinalLF();

            indexing_data->addAll( 0, 0, line_position, encodingGuess );
        }
    }
    else {
        // TODO: Check that the file is seekable?
        // If the file cannot be open, we do as if it was empty
        LOG(logWARNING) << "Cannot open file " << fileName_.toStdString();

        indexing_data->clear();

        emit indexingProgressed( 100 );
    }
    LOG(logINFO) << "Detected encoding " << fileTextCodec->name().data();
}

// Called in the worker thread's context
bool FullIndexOperation::start()
{
    LOG(logDEBUG) << "FullIndexOperation::start(), file "
        << fileName_.toStdString();

    LOG(logDEBUG) << "FullIndexOperation: Starting the count...";

    emit indexingProgressed( 0 );

    // First empty the index
    indexing_data_->clear();
    indexing_data_->forceEncoding(forcedEncoding_);

    doIndex( indexing_data_, 0 );

    LOG(logDEBUG) << "FullIndexOperation: ... finished counting."
        "interrupt = " << static_cast<bool>(*interruptRequest_);

    return ( *interruptRequest_ ? false : true );
}

bool PartialIndexOperation::start()
{
    LOG(logDEBUG) << "PartialIndexOperation::start(), file "
        << fileName_.toStdString();

    LOG(logDEBUG) << "PartialIndexOperation: Starting the count at "
        << initialPosition_ << " ...";

    emit indexingProgressed( 0 );

    doIndex( indexing_data_, initialPosition_ );

    LOG(logDEBUG) << "PartialIndexOperation: ... finished counting.";

    return ( *interruptRequest_ ? false : true );
}

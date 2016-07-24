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

#ifndef LOGDATAWORKERTHREAD_H
#define LOGDATAWORKERTHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QVector>
#include <QTextCodec>

#include "loadingstatus.h"
#include "linepositionarray.h"

#include "utils.h"
#include "atomicflag.h"

struct EncodingParameters
{
    EncodingParameters():lineFeedWidth(1),lineFeedIndex(0){}
    explicit EncodingParameters(const QTextCodec* codec);

    int lineFeedWidth;
    int lineFeedIndex;

    bool operator ==(const EncodingParameters& other) const
    {
        return  lineFeedWidth == other.lineFeedWidth &&
                lineFeedIndex == other.lineFeedIndex;
    }

    bool operator !=(const EncodingParameters& other) const
    {
        return !operator ==(other);
    }
};

// This class is a thread-safe set of indexing data.
class IndexingData
{
  public:
    IndexingData() : dataMutex_(), linePosition_(), maxLength_(0),
        indexedSize_(0),
        encodingGuess_(QTextCodec::codecForLocale()),
        encodingForced_(nullptr)
    { }

    // Get the total indexed size
    qint64 getSize() const;

    // Get the length of the longest line
    int getMaxLength() const;

    // Get the total number of lines
    LineNumber getNbLines() const;

    // Get the position (in byte from the beginning of the file)
    // of the end of the passed line.
    qint64 getPosForLine( LineNumber line ) const;

    // Get the guessed encoding for the content.
    QTextCodec* getEncodingGuess() const;

    QTextCodec* getForcedEncoding() const;
    void forceEncoding(QTextCodec* codec);


    // Atomically add to all the existing
    // indexing data.
    void addAll( qint64 size, int length,
            const FastLinePositionArray& linePosition,
            QTextCodec* encoding );

    // Completely clear the indexing data.
    void clear();

  private:
    mutable QMutex dataMutex_;

    LinePositionArray linePosition_;
    int maxLength_;
    qint64 indexedSize_;

    QTextCodec* encodingGuess_;
    QTextCodec* encodingForced_;
};

class IndexOperation : public QObject
{
  Q_OBJECT
  public:
    IndexOperation(const QString& fileName,
            IndexingData* indexingData, AtomicFlag* interruptRequest);

    virtual ~IndexOperation() { }

    // Start the indexing operation, returns true if it has been done
    // and false if it has been cancelled (results not copied)
    virtual bool start() = 0;

  signals:
    void indexingProgressed( int );

  protected:
    static const int sizeChunk;

    // Returns the total size indexed
    // Modify the passed linePosition and maxLength
    void doIndex( IndexingData* linePosition, qint64 initialPosition );

    QString fileName_;
    AtomicFlag* interruptRequest_;
    IndexingData* indexing_data_;
};

class FullIndexOperation : public IndexOperation
{
  public:
    FullIndexOperation( const QString& fileName,
            IndexingData* indexingData, AtomicFlag* interruptRequest,
            QTextCodec* forcedEncoding = nullptr)
        : IndexOperation( fileName, indexingData, interruptRequest )
        , forcedEncoding_(forcedEncoding)
    { }
    virtual bool start();
  private:
    QTextCodec* forcedEncoding_;
};

class PartialIndexOperation : public IndexOperation
{
  public:
    PartialIndexOperation( const QString& fileName, IndexingData* indexingData,
            AtomicFlag* interruptRequest, qint64 position );
    virtual bool start();

  private:
    qint64 initialPosition_;
};

// Create and manage the thread doing loading/indexing for
// the creating LogData. One LogDataWorkerThread is used
// per LogData instance.
// Note everything except the run() function is in the LogData's
// thread.
class LogDataWorkerThread : public QThread
{
  Q_OBJECT

  public:
    // Pass a pointer to the IndexingData (initially empty)
    // This object will change it when indexing (IndexingData must be thread safe!)
    LogDataWorkerThread( IndexingData* indexing_data );
    ~LogDataWorkerThread();

    // Attaches to a file on disk. Attaching to a non existant file
    // will work, it will just appear as an empty file.
    void attachFile( const QString& fileName );
    // Instructs the thread to start a new full indexing of the file, sending
    // signals as it progresses.
    void indexAll(QTextCodec *forcedEncoding = nullptr);
    // Instructs the thread to start a partial indexing (starting at
    // the index passed).
    void indexAdditionalLines( qint64 position );
    // Interrupts the indexing if one is in progress
    void interrupt();

    // Returns a copy of the current indexing data
    void getIndexingData( qint64* indexedSize,
            int* maxLength, LinePositionArray* linePosition );

  signals:
    // Sent during the indexing process to signal progress
    // percent being the percentage of completion.
    void indexingProgressed( int percent );
    // Sent when indexing is finished, signals the client
    // to copy the new data back.
    void indexingFinished( LoadingStatus status );

  protected:
    void run();

  private:
    void doIndexAll();

    // Mutex to protect operationRequested_ and friends
    QMutex mutex_;
    QWaitCondition operationRequestedCond_;
    QWaitCondition nothingToDoCond_;
    QString fileName_;

    // Set when the thread must die
    AtomicFlag terminate_;
    AtomicFlag interruptRequested_;
    IndexOperation* operationRequested_;

    // Pointer to the owner's indexing data (we modify it)
    IndexingData* indexing_data_;
};

#endif

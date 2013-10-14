/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

// This class is a list of end of lines position,
// in addition to a list of qint64 (positions within the files)
// it can keep track of whether the final LF was added (for non-LF terminated
// files) and remove it when more data are added.
class LinePositionArray
{
  public:
    // Default constructor
    LinePositionArray() : array()
    { fakeFinalLF_ = false; }
    // Copy constructor
    inline LinePositionArray( const LinePositionArray& orig )
        : array(orig.array)
    { fakeFinalLF_ = orig.fakeFinalLF_; }

    // Add a new line position at the given position
    inline void append( qint64 pos )
    { array.append( pos ); }
    // Size of the array
    inline int size()
    { return array.size(); }
    // Extract an element
    inline qint64 at( int i ) const
    { return array.at( i ); }
    inline qint64 operator[]( int i ) const
    { return array.at( i ); }
    // Set the presence of a fake final LF
    // Must be used after 'append'-ing a fake LF at the end.
    void setFakeFinalLF( bool finalLF=true )
    { fakeFinalLF_ = finalLF; }

    // Add another list to this one, removing any fake LF on this list.
    LinePositionArray& operator+= ( const LinePositionArray& other )
    {
        // If our final LF is fake, we remove it
        if ( fakeFinalLF_ )
            this->array.pop_back();

        // Append the arrays
        this->array += other.array;

        // In case the 'other' object has a fake LF
        this->fakeFinalLF_ = other.fakeFinalLF_;

        return *this;
    }

  private:
    QVector<qint64> array;
    bool fakeFinalLF_;
};

// This class is a mutex protected set of indexing data.
// It is thread safe.
class IndexingData
{
  public:
    IndexingData() : dataMutex_(), linePosition_(), maxLength_(0), indexedSize_(0) { }

    // Atomically get all the indexing data
    void getAll( qint64* size, int* length,
            LinePositionArray* linePosition );

    // Atomically set all the indexing data
    // (overwriting the existing)
    void setAll( qint64 size, int length,
            const LinePositionArray& linePosition );

    // Atomically add to all the existing 
    // indexing data.
    void addAll( qint64 size, int length,
            const LinePositionArray& linePosition );

  private:
    QMutex dataMutex_;

    LinePositionArray linePosition_;
    int maxLength_;
    qint64 indexedSize_;
};

class IndexOperation : public QObject
{
  Q_OBJECT
  public:
    IndexOperation( QString& fileName, bool* interruptRequest );

    virtual ~IndexOperation() { }

    // Start the indexing operation, returns true if it has been done
    // and false if it has been cancelled (results not copied)
    virtual bool start( IndexingData& result ) = 0;

  signals:
    void indexingProgressed( int );

  protected:
    static const int sizeChunk;

    // Returns the total size indexed
    qint64 doIndex( LinePositionArray& linePosition, int* maxLength,
            qint64 initialPosition );

    QString fileName_;
    bool* interruptRequest_;
};

class FullIndexOperation : public IndexOperation
{
  public:
    FullIndexOperation( QString& fileName, bool* interruptRequest )
        : IndexOperation( fileName, interruptRequest ) { }
    virtual bool start( IndexingData& result );
};

class PartialIndexOperation : public IndexOperation
{
  public:
    PartialIndexOperation( QString& fileName, bool* interruptRequest, qint64 position );
    virtual bool start( IndexingData& result );

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
    LogDataWorkerThread();
    ~LogDataWorkerThread();

    // Attaches to a file on disk. Attaching to a non existant file
    // will work, it will just appear as an empty file.
    void attachFile( const QString& fileName );
    // Instructs the thread to start a new full indexing of the file, sending
    // signals as it progresses.
    void indexAll();
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
    void indexingFinished( bool success );

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
    bool terminate_;
    bool interruptRequested_;
    IndexOperation* operationRequested_;

    // Shared indexing data
    IndexingData indexingData_;
};

#endif

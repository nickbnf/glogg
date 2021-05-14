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
 * Copyright (C) 2021 Anton Filimonov and other contributors
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

#ifndef LOGDATAOPERATION_H
#define LOGDATAOPERATION_H

#include <variant>

#include "logdataworker.h"

#include "synchronization.h"

// This class models an indexing operation.
// It exists to permit LogData to delay the operation if another
// one is ongoing (operations are asynchronous)
class LogDataOperation {
  public:
    LogDataOperation() = default;
    explicit LogDataOperation( const QString& fileName )
        : filename_( fileName )
    {
    }

    // Permit each child to have its destructor
    virtual ~LogDataOperation() = default;

    void start( LogDataWorker* workerThread ) const
    {
        doStart( *workerThread );
    }
    const QString& getFilename() const
    {
        return filename_;
    }

  protected:
    virtual void doStart( LogDataWorker& workerThread ) const = 0;
    QString filename_;
};

// Attaching a new file (change name + full index)
class AttachOperation : public LogDataOperation {
  public:
    explicit AttachOperation( const QString& fileName )
        : LogDataOperation( fileName )
    {
    }

  protected:
    void doStart( LogDataWorker& workerThread ) const override;
};

// Reindexing the current file
class FullReindexOperation : public LogDataOperation {
  public:
    explicit FullReindexOperation( QTextCodec* forcedEncoding = nullptr )
        : forcedEncoding_( forcedEncoding )
    {
    }

  protected:
    void doStart( LogDataWorker& workerThread ) const override;

  private:
    QTextCodec* forcedEncoding_;
};

// Indexing part of the current file (from fileSize)
class PartialReindexOperation : public LogDataOperation {
  protected:
    void doStart( LogDataWorker& workerThread ) const override;
};

// Attaching a new file (change name + full index)
class CheckDataChangesOperation : public LogDataOperation {
  protected:
    void doStart( LogDataWorker& workerThread ) const override;
};

class OperationQueue {
  public:
    explicit OperationQueue( std::function<void()> beforeOperationStart );

    void setWorker( std::unique_ptr<LogDataWorker>&& worker );

    void interrupt();
    void shutdown();

    template <typename Op, typename... Args>
    void enqueueOperation( Args&&... args )
    {
        enqueueOperation( Op{ std::forward<Args>( args )... } );
    }

    void finishOperationAndStartNext();

  private:
    using OperationVariant = std::variant<std::monostate, AttachOperation, FullReindexOperation,
                                           PartialReindexOperation, CheckDataChangesOperation>;

    void enqueueOperation( OperationVariant&& operation );
    void tryStartOperation( OperationVariant&& operation );

    std::function<void()> beforeOperationStart_;

  private:
    mutable Mutex mutex_;
    
    OperationVariant executingOperation_;
    OperationVariant pendingOperation_;

    std::unique_ptr<LogDataWorker> worker_;
};

#endif
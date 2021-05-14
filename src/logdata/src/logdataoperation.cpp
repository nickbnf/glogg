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

#include "logdataoperation.h"

#include "log.h"
#include "overload_visitor.h"
#include "synchronization.h"

void AttachOperation::doStart( LogDataWorker& workerThread ) const
{
    LOG_INFO << "Attaching " << filename_.toStdString();
    workerThread.attachFile( filename_ );
    workerThread.indexAll();
}

void FullReindexOperation::doStart( LogDataWorker& workerThread ) const
{
    LOG_INFO << "Reindexing (full)";
    workerThread.indexAll( forcedEncoding_ );
}

void PartialReindexOperation::doStart( LogDataWorker& workerThread ) const
{
    LOG_INFO << "Reindexing (partial)";
    workerThread.indexAdditionalLines();
}

void CheckDataChangesOperation::doStart( LogDataWorker& workerThread ) const
{
    LOG_INFO << "Checking file changes";
    workerThread.checkFileChanges();
}

OperationQueue::OperationQueue( std::function<void()> beforeOperationStart )
    : beforeOperationStart_( std::move( beforeOperationStart ) )
{
}

void OperationQueue::setWorker( std::unique_ptr<LogDataWorker>&& worker )
{
    worker_ = std::move( worker );
}

void OperationQueue::interrupt()
{
    ScopedLock guard( mutex_ );
    if ( worker_ ) {
        worker_->interrupt();
    }
}

void OperationQueue::shutdown()
{
    ScopedLock guard( mutex_ );
    if ( auto worker = std::move( worker_ ) ) {
        worker->interrupt();
    }

    LOG_INFO << "Operation queue shutdown";
}

void OperationQueue::tryStartOperation( OperationVariant&& operation )
{
    if ( !worker_ ) {
        LOG_WARNING << "No worker for operation";
        executingOperation_ = {};
        return;
    }

    std::swap( executingOperation_, operation );
    absl::visit( makeOverloadVisitor(
                     [this]( const LogDataOperation& logDataOperation ) {
                         beforeOperationStart_();
                         logDataOperation.start( worker_.get() );
                         LOG_INFO << "Started operation " << executingOperation_.index();
                     },
                     []( absl::monostate ) { LOG_INFO << "no operation to start"; } ),
                 executingOperation_ );
}

void OperationQueue::enqueueOperation( OperationVariant&& operation )
{
    ScopedLock guard( mutex_ );

    LOG_INFO << "Enqueue operation " << operation.index() << ", now executing "
             << executingOperation_.index();

    if ( executingOperation_.index() > 0 ) {
        pendingOperation_ = std::move( operation );
    }
    else {
        tryStartOperation( std::move( operation ) );
    }
}

void OperationQueue::finishOperationAndStartNext()
{
    ScopedLock guard( mutex_ );
    LOG_INFO << "Finished operation " << executingOperation_.index() << ", next operation "
             << pendingOperation_.index();

    tryStartOperation( std::exchange( pendingOperation_, {} ) );
}
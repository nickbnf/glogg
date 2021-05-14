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

#include <memory>

#include <QFileInfo>
#include <QMimeDatabase>
#include <QtConcurrent>

#include <k7zip.h>
#include <kcompressiondevice.h>
#include <ktar.h>
#include <kzip.h>

#include "log.h"

#include "decompressor.h"

namespace {

enum class Archive { None, Zip7, Tar, Zip, Gz, Bz2, Xz };

Archive archiveTypeByExtension( const QString& archiveFilePath )
{
    const auto info = QFileInfo( archiveFilePath );
    const auto extension = info.suffix().toLower();
    LOG_INFO << "Suffix is " << extension;

    if ( extension == "zip" ) {
        return Archive::Zip;
    }
    else if ( extension == "7z" ) {
        return Archive::Zip7;
    }
    else if ( extension == "tgz" || extension == "tbz2" || extension == "txz" ) {
        return Archive::Tar;
    }

    if ( extension == "gz" || extension == "bz2" || extension == "xz" || extension == "lzma" ) {
        const auto completeSuffix = info.completeSuffix().toLower();
        if ( completeSuffix.contains( "tar." ) ) {
            return Archive::Tar;
        }
        else if ( extension == "gz" ) {
            return Archive::Gz;
        }
        else if ( extension == "bz2" ) {
            return Archive::Bz2;
        }
        else if ( extension == "xz" || extension == "lzma" ) {
            return Archive::Xz;
        }
    }

    return Archive::None;
}

Archive archiveType( const QString& archiveFilePath )
{
    QMimeDatabase mimeDb;
    const auto mime = mimeDb.mimeTypeForFile( archiveFilePath );

    if ( mime.inherits( "application/x-7z-compressed" ) ) {
        return Archive::Zip7;
    }

    if ( mime.inherits( "application/zip" ) ) {
        return Archive::Zip;
    }

    if ( mime.inherits( "application/x-tar" ) ) {
        return Archive::Tar;
    }

    auto mimeArchiveType = Archive::None;

    if ( mime.inherits( "application/x-gzip" ) ) {
        mimeArchiveType = Archive::Gz;
    }
    else if ( mime.inherits( "application/x-bzip" ) ) {
        mimeArchiveType = Archive::Bz2;
    }
    else if ( mime.inherits( "application/x-lzma" ) || mime.inherits( "application/x-xz" ) ) {
        mimeArchiveType = Archive::Xz;
    }

    if ( mimeArchiveType == Archive::None ) {
        return archiveTypeByExtension( archiveFilePath );
    }

    const auto info = QFileInfo( archiveFilePath );
    const auto extension = info.suffix().toLower();
    const auto completeSuffix = info.completeSuffix().toLower();
    if ( completeSuffix.contains( "tar.", Qt::CaseInsensitive )
         || extension.endsWith( "tgz", Qt::CaseInsensitive )
         || extension.endsWith( "tbz", Qt::CaseInsensitive )
         || extension.endsWith( "tbz2", Qt::CaseInsensitive )
         || extension.endsWith( "txz", Qt::CaseInsensitive ) ) {

        return Archive::Tar;
    }

    return mimeArchiveType;
}

std::shared_ptr<KArchive> makeExtractor( Archive archiveType, const QString& archiveFilePath )
{
    switch ( archiveType ) {
    case Archive::Zip:
        return std::make_shared<KZip>( archiveFilePath );
    case Archive::Zip7:
        return std::make_shared<K7Zip>( archiveFilePath );
    case Archive::Tar:
        return std::make_shared<KTar>( archiveFilePath );
    default:
        return {};
    }
}

std::shared_ptr<KCompressionDevice> makeDecompressor( Archive archiveType,
                                                      const QString& archiveFilePath )
{
    KCompressionDevice::CompressionType compression = KCompressionDevice::None;

    switch ( archiveType ) {
    case Archive::Gz:
        compression = KCompressionDevice::GZip;
        break;
    case Archive::Bz2:
        compression = KCompressionDevice::BZip2;
        break;
    case Archive::Xz:
        compression = KCompressionDevice::Xz;
        break;
    default:
        compression = KCompressionDevice::None;
    }

    if ( compression != KCompressionDevice::None ) {
        return std::make_shared<KCompressionDevice>( archiveFilePath, compression );
    }
    else {
        return {};
    }
}

bool doExtract( std::shared_ptr<KArchive> archive, const QString& archiveFilePath,
                const QString& destination, AtomicFlag& interrupt )
{
    if ( !archive->open( QIODevice::ReadOnly ) ) {
        LOG_WARNING << "Cannot open " << archiveFilePath;
        return false;
    }

    const KArchiveDirectory* root = archive->directory();

    if ( !root ) {
        LOG_WARNING << "Cannot open root directory" << archiveFilePath;
        archive->close();
        return false;
    }

    auto result = false;
    try {
        const auto recursive = true;
#ifdef KLOGG_KARCHIVE
        result = root->copyTo( destination, interrupt, recursive );
#else
        result = root->copyTo( destination, recursive );
#endif
    } catch ( const std::exception& e ) {
        LOG_ERROR << "Exception during extract: " << e.what();
    }

    if ( interrupt ) {
        result = false;
        LOG_INFO << "Interrupted extract of " << archiveFilePath;
    }

    archive->close();
    return result;
}

bool doDecompress( std::shared_ptr<KCompressionDevice> input, const QString& archiveFilePath,
                   QFile* outputFile, AtomicFlag& interrupt )
{
    if ( !input->open( QIODevice::ReadOnly ) ) {
        LOG_WARNING << "Cannot open " << archiveFilePath;
        return false;
    }

    bool success = true;
    try {
        while ( !input->atEnd() ) {
            if ( interrupt ) {
                success = false;
                LOG_INFO << "Interrupted decompress of " << archiveFilePath;
                break;
            }

            QByteArray data = input->read( 4 * 1024 * 1024 );
            if ( data.size() > 0 ) {
                const auto writtenBytes = outputFile->write( data );
                if ( writtenBytes < 0 ) {
                    LOG_ERROR << "Error decompressing " << archiveFilePath;
                    success = false;
                    break;
                }
            }
        }
    } catch ( const std::exception& e ) {
        LOG_ERROR << "Exception during decompress: " << e.what();
    }

    input->close();
    outputFile->close();

    return success;
}

} // namespace

Decompressor::Decompressor( QObject* parent )
    : QObject( parent )
{
    connect( &watcher_, &QFutureWatcher<bool>::finished, [ this ]() {
        LOG_INFO << "Decompressor finished " << watcher_.result();
        emit finished( watcher_.result() );
    } );
}

bool Decompressor::waitForResult()
{
    return watcher_.result();
}

DecompressAction Decompressor::action( const QString& archiveFilePath )
{
    const auto archive = archiveType( archiveFilePath );

    switch ( archive ) {
    case Archive::Zip:
    case Archive::Zip7:
    case Archive::Tar:
        return DecompressAction::Extract;
    case Archive::Gz:
    case Archive::Bz2:
    case Archive::Xz:
        return DecompressAction::Decompress;
    default:
        return DecompressAction::None;
    }
}

bool Decompressor::decompress( const QString& archiveFilePath, QFile* outputFile,
                               AtomicFlag& interrupt )
{
    auto decompressor = makeDecompressor( archiveType( archiveFilePath ), archiveFilePath );
    if ( !decompressor ) {
        LOG_WARNING << "Unsupported archive " << archiveFilePath.constData();
        return false;
    }

    future_ = QtConcurrent::run(
        [ input = std::move( decompressor ), archiveFilePath, outputFile, &interrupt ] {
            return doDecompress( input, archiveFilePath, outputFile, interrupt );
        } );
    watcher_.setFuture( future_ );

    return true;
}

bool Decompressor::extract( const QString& archiveFilePath, const QString& destination,
                            AtomicFlag& interrupt )
{
    auto archive = makeExtractor( archiveType( archiveFilePath ), archiveFilePath );
    if ( !archive ) {
        LOG_WARNING << "Unsupported archive " << archiveFilePath;
        return false;
    }

    // Open the archive

    future_ = QtConcurrent::run(
        [ ar = std::move( archive ), archiveFilePath, destination, &interrupt ] {
            return doExtract( ar, archiveFilePath, destination, interrupt );
        } );
    watcher_.setFuture( future_ );

    return true;
}

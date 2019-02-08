/*
 * Copyright (C) 2019 Anton Filimonov
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

#include "log.h"
#include <QtCore/QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/stat.h>
#endif

struct FileId {
    uint64_t fileIndex;
    uint64_t volumeIndex;

    bool operator!=( const FileId& other ) const
    {
        return fileIndex != other.fileIndex || volumeIndex != other.volumeIndex;
    }
};

inline FileId getFileId( const QString& filename )
{
#ifdef Q_OS_WIN
    DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    int accessRights = GENERIC_READ;
    DWORD creationDisp = OPEN_EXISTING;

    // Create the file handle.
    SECURITY_ATTRIBUTES securityAtts = { sizeof( SECURITY_ATTRIBUTES ), NULL, FALSE };

    HANDLE fileHandle
        = CreateFileW( (const wchar_t*)filename.utf16(), accessRights, shareMode, &securityAtts,
                       creationDisp, FILE_FLAG_BACKUP_SEMANTICS, NULL );

    if ( fileHandle == INVALID_HANDLE_VALUE ) {
        LOG( logDEBUG ) << "Failed to get file info for " << filename.toStdString() << ", gle "
                        << ::GetLastError();
        return FileId{};
    }

    using FileHandleGuard = std::unique_ptr<void, decltype( &CloseHandle )>;
    auto fileHandleGuard = FileHandleGuard{ fileHandle, CloseHandle };

    BY_HANDLE_FILE_INFORMATION info;
    if ( !::GetFileInformationByHandle( fileHandle, &info ) ) {
        LOG( logDEBUG ) << "Failed to get file info for " << filename.toStdString() << ", gle "
                        << ::GetLastError();
        return FileId{};
    }

    ULARGE_INTEGER fileIndex = { info.nFileIndexLow, info.nFileIndexHigh };
    return FileId{ fileIndex.QuadPart, static_cast<uint64_t>( info.dwVolumeSerialNumber ) };
#else
    struct stat info;
    if ( lstat( filename.toUtf8().constData(), &info ) != 0 ) {
        LOG( logDEBUG ) << "Failed to get file info for " << filename.toStdString();
        return FileId{};
    }

    return FileId{ info.st_ino, static_cast<uint64_t>( info.st_dev ) };
#endif
}

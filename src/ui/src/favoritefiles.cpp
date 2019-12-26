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

#include <cmath>

#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include "favoritefiles.h"
#include "log.h"

namespace {
constexpr const int FavoriteFilesVersion = 1;
constexpr const int MaxPathLength = 64;

// inspired by http://chadkuehn.com/shrink-file-paths-with-an-ellipsis-in-c/
QString shrinkPath( QString fullPath, int limit, QString delimiter = "…" )
{
    if ( fullPath.isEmpty() ) {
        return fullPath;
    }

    const auto fileInfo = QFileInfo( fullPath );
    const auto fileName = fileInfo.fileName();
    const auto absoluteNativePath = QDir::toNativeSeparators( fileInfo.absolutePath() );

    const auto idealMinLength = fileName.length() + delimiter.length();

    // less than the minimum amt
    if ( limit < ( ( 2 * delimiter.length() ) + 1 ) ) {
        return "";
    }

    // fullpath
    if ( limit >= fullPath.length() ) {
        return QDir::toNativeSeparators( fullPath );
    }

    // file name condensing
    if ( limit < idealMinLength ) {
        return delimiter + fileName.mid( 0, ( limit - ( 2 * delimiter.length() ) ) ) + delimiter;
    }

    // whole name only, no folder structure shown
    if ( limit == idealMinLength ) {
        return delimiter + fileName;
    }

    return absoluteNativePath.mid( 0, ( limit - ( idealMinLength + 1 ) ) ) + delimiter + QDir::separator() + fileName;
}

} // namespace

FavoriteFiles::File::File( const QString& path )
    : fullPath( path )
    , fullPathNative( QDir::toNativeSeparators( path ) )
{
    const auto fileInfo = QFileInfo( path );
    displayName = shrinkPath( fullPath, MaxPathLength );
}

struct DisplayNameComparator {
    bool operator()( const FavoriteFiles::File& lhs, const FavoriteFiles::File& rhs )
    {
        return lhs.displayName < rhs.displayName;
    }
};

void FavoriteFiles::add( const QString& path )
{
    if ( std::any_of( files_.begin(), files_.end(), FullPathComparator( path ) ) ) {
        return;
    }

    auto newFile = FavoriteFiles::File( path );

    auto lowerBound
        = std::lower_bound( files_.begin(), files_.end(), newFile, DisplayNameComparator{} );

    files_.insert( lowerBound, newFile );
}

void FavoriteFiles::remove( const QString& path )
{
    auto existingFile = std::find_if( files_.begin(), files_.end(), FullPathComparator( path ) );
    files_.erase( existingFile );
}

std::vector<FavoriteFiles::File> FavoriteFiles::favorites() const
{
    return files_;
}

void FavoriteFiles::saveToStorage( QSettings& settings ) const
{
    LOG( logDEBUG ) << "FavoriteFiles::saveToStorage";

    settings.beginGroup( "FavoriteFiles" );
    settings.setValue( "version", FavoriteFilesVersion );
    settings.remove( "favorites" );
    settings.beginWriteArray( "favorites" );
    for ( auto i = 0u; i < files_.size(); ++i ) {
        settings.setArrayIndex( i );
        settings.setValue( "name", files_.at( i ).fullPath );
    }
    settings.endArray();
    settings.endGroup();
}

void FavoriteFiles::retrieveFromStorage( QSettings& settings )
{
    LOG( logDEBUG ) << "FavoriteFiles::retrieveFromStorage";

    files_.clear();

    if ( settings.contains( "FavoriteFiles/version" ) ) {
        settings.beginGroup( "FavoriteFiles" );
        if ( settings.value( "version" ) == FavoriteFilesVersion ) {
            int size = settings.beginReadArray( "favorites" );
            for ( int i = 0; i < size; ++i ) {
                settings.setArrayIndex( i );
                QString path = settings.value( "name" ).toString();
                files_.emplace_back( path );
            }
            settings.endArray();
        }
        else {
            LOG( logERROR ) << "Unknown version of favorite files, ignoring it...";
        }
        settings.endGroup();
    }
}

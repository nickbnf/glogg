/*
 * Copyright (C) 2009, 2010, 2013, 2015 Nicolas Bonnefon and other contributors
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

#include <QFontInfo>
#include <QStyleFactory>

#include "log.h"

#include "configuration.h"

Configuration::Configuration()
{
    mainFont_.setStyleHint( QFont::Courier, QFont::PreferOutline );

    QFontInfo fi( mainFont_ );
    LOG( logDEBUG ) << "Default font is " << fi.family().toStdString();

    splitterSizes_ << 400 << 100;
}

// Accessor functions
QFont Configuration::mainFont() const
{
    return mainFont_;
}

void Configuration::setMainFont( QFont newFont )
{
    LOG( logDEBUG ) << "Configuration::setMainFont";

    mainFont_ = newFont;
}

void Configuration::retrieveFromStorage( QSettings& settings )
{
    LOG( logDEBUG ) << "Configuration::retrieveFromStorage";

    static const Configuration Default;

    // Fonts
    QString family = settings.value( "mainFont.family", Default.mainFont_.family() ).toString();
    int size = settings.value( "mainFont.size", Default.mainFont_.pointSize() ).toInt();

    // If no config read, keep the default
    if ( !family.isNull() )
        mainFont_ = QFont( family, size );

    forceFontAntialiasing_
        = settings.value( "mainFont.antialiasing", Default.forceFontAntialiasing_ ).toBool();

    enableQtHighDpi_ = settings.value( "view.qtHiDpi", Default.enableQtHighDpi_ ).toBool();

    scaleFactorRounding_
        = settings.value( "view.scaleFactorRounding", Default.scaleFactorRounding_ ).toInt();

    // Regexp types
    mainRegexpType_ = static_cast<SearchRegexpType>(
        settings.value( "regexpType.main", Default.mainRegexpType_ ).toInt() );
    quickfindRegexpType_ = static_cast<SearchRegexpType>(
        settings.value( "regexpType.quickfind", Default.quickfindRegexpType_ ).toInt() );
    quickfindIncremental_
        = settings.value( "quickfind.incremental", Default.quickfindIncremental_ ).toBool();

    // "Advanced" settings
    nativeFileWatchEnabled_
        = settings.value( "nativeFileWatch.enabled", Default.nativeFileWatchEnabled_ ).toBool();
    settings.remove( "nativeFileWatch.enabled" );
    nativeFileWatchEnabled_
        = settings.value( "filewatch.useNative", nativeFileWatchEnabled_ ).toBool();

    pollingEnabled_ = settings.value( "polling.enabled", Default.pollingEnabled_ ).toBool();
    settings.remove( "polling.enabled" );
    pollingEnabled_ = settings.value( "filewatch.usePolling", pollingEnabled_ ).toBool();

    pollIntervalMs_ = settings.value( "polling.intervalMs", Default.pollIntervalMs_ ).toInt();
    settings.remove( "polling.intervalMs" );
    pollIntervalMs_ = settings.value( "filewatch.pollingIntervalMs", pollIntervalMs_ ).toInt();

    fastModificationDetection_
        = settings
              .value( "filewatch.fastModificationDetection", Default.fastModificationDetection_ )
              .toBool();

    loadLastSession_ = settings.value( "session.loadLast", Default.loadLastSession_ ).toBool();
    allowMultipleWindows_
        = settings.value( "session.multipleWindows", Default.allowMultipleWindows_ ).toBool();
    followFileOnLoad_
        = settings.value( "session.followOnLoad", Default.followFileOnLoad_ ).toBool();

    enableLogging_ = settings.value( "logging.enableLogging", Default.enableLogging_ ).toBool();
    loggingLevel_ = static_cast<uint8_t>(
        settings.value( "logging.verbosity", Default.loggingLevel_ ).toInt() );

    enableVersionChecking_
        = settings.value( "versionchecker.enabled", Default.enableVersionChecking_ ).toBool();

    extractArchives_ = settings.value( "archives.extract", Default.extractArchives_ ).toBool();
    extractArchivesAlways_
        = settings.value( "archives.extractAlways", Default.extractArchivesAlways_ ).toBool();

    // "Perf" settings
    useParallelSearch_
        = settings.value( "perf.useParallelSearch", Default.useParallelSearch_ ).toBool();
    useSearchResultsCache_
        = settings.value( "perf.useSearchResultsCache", Default.useSearchResultsCache_ ).toBool();
    searchResultsCacheLines_
        = settings.value( "perf.searchResultsCacheLines", Default.searchResultsCacheLines_ )
              .toInt();
    indexReadBufferSizeMb_
        = settings.value( "perf.indexReadBufferSizeMb", Default.indexReadBufferSizeMb_ ).toInt();
    searchReadBufferSizeLines_
        = settings.value( "perf.searchReadBufferSizeLines", Default.searchReadBufferSizeLines_ )
              .toInt();
    searchThreadPoolSize_
        = settings.value( "perf.searchThreadPoolSize", Default.searchThreadPoolSize_ ).toInt();
    keepFileClosed_ = settings.value( "perf.keepFileClosed", Default.keepFileClosed_ ).toBool();
    useLineEndingCache_
        = settings.value( "perf.useLineEndingCache", Default.useLineEndingCache_ ).toBool();

    verifySslPeers_ = settings.value( "net.verifySslPeers", Default.verifySslPeers_ ).toBool();

    // View settings
    overviewVisible_ = settings.value( "view.overviewVisible", Default.overviewVisible_ ).toBool();
    lineNumbersVisibleInMain_
        = settings.value( "view.lineNumbersVisibleInMain", Default.lineNumbersVisibleInMain_ )
              .toBool();
    lineNumbersVisibleInFiltered_
        = settings
              .value( "view.lineNumbersVisibleInFiltered", Default.lineNumbersVisibleInFiltered_ )
              .toBool();
    minimizeToTray_ = settings.value( "view.minimizeToTray", Default.minimizeToTray_ ).toBool();

    style_ = settings.value( "view.style", Default.style_ ).toString();

    auto availableStyles = QStyleFactory::keys();
    availableStyles << DarkStyleKey;
    if ( !availableStyles.contains( style_ ) ) {
        style_ = availableStyles.front();
    }

    // Some sanity check (mainly for people upgrading)
    if ( quickfindIncremental_ )
        quickfindRegexpType_ = FixedString;

    // Default crawler settings
    searchAutoRefresh_
        = settings.value( "defaultView.searchAutoRefresh", Default.searchAutoRefresh_ ).toBool();
    searchIgnoreCase_
        = settings.value( "defaultView.searchIgnoreCase", Default.searchIgnoreCase_ ).toBool();

    if ( settings.contains( "defaultView.splitterSizes" ) ) {
        splitterSizes_.clear();

        const auto sizes = settings.value( "defaultView.splitterSizes" ).toList();
        std::transform( sizes.begin(), sizes.end(), std::back_inserter( splitterSizes_ ),
                        []( auto v ) { return v.toInt(); } );
    }
}

void Configuration::saveToStorage( QSettings& settings ) const
{
    LOG( logDEBUG ) << "Configuration::saveToStorage";

    QFontInfo fi( mainFont_ );

    settings.setValue( "mainFont.family", fi.family() );
    settings.setValue( "mainFont.size", fi.pointSize() );
    settings.setValue( "mainFont.antialiasing", forceFontAntialiasing_ );
    settings.setValue( "regexpType.main", static_cast<int>( mainRegexpType_ ) );
    settings.setValue( "regexpType.quickfind", static_cast<int>( quickfindRegexpType_ ) );
    settings.setValue( "quickfind.incremental", quickfindIncremental_ );
    settings.setValue( "filewatch.useNative", nativeFileWatchEnabled_ );
    settings.setValue( "filewatch.usePolling", pollingEnabled_ );
    settings.setValue( "filewatch.pollingIntervalMs", pollIntervalMs_ );
    settings.setValue( "filewatch.fastModificationDetection", fastModificationDetection_ );
    settings.setValue( "session.loadLast", loadLastSession_ );
    settings.setValue( "session.multipleWindows", allowMultipleWindows_ );
    settings.setValue( "session.followOnLoad", followFileOnLoad_ );

    settings.setValue( "logging.enableLogging", enableLogging_ );
    settings.setValue( "logging.verbosity", loggingLevel_ );

    settings.setValue( "versionchecker.enabled", enableVersionChecking_ );

    settings.setValue( "archives.extract", extractArchives_ );
    settings.setValue( "archives.extractAlways", extractArchivesAlways_ );

    settings.setValue( "perf.useParallelSearch", useParallelSearch_ );
    settings.setValue( "perf.useSearchResultsCache", useSearchResultsCache_ );
    settings.setValue( "perf.searchResultsCacheLines", searchResultsCacheLines_ );
    settings.setValue( "perf.indexReadBufferSizeMb", indexReadBufferSizeMb_ );
    settings.setValue( "perf.searchReadBufferSizeLines", searchReadBufferSizeLines_ );
    settings.setValue( "perf.searchThreadPoolSize", searchThreadPoolSize_ );
    settings.setValue( "perf.keepFileClosed", keepFileClosed_ );
    settings.setValue( "perf.useLineEndingCache", useLineEndingCache_ );

    settings.setValue( "net.verifySslPeers", verifySslPeers_ );

    settings.setValue( "view.overviewVisible", overviewVisible_ );
    settings.setValue( "view.lineNumbersVisibleInMain", lineNumbersVisibleInMain_ );
    settings.setValue( "view.lineNumbersVisibleInFiltered", lineNumbersVisibleInFiltered_ );
    settings.setValue( "view.minimizeToTray", minimizeToTray_ );
    settings.setValue( "view.style", style_ );

    settings.setValue( "view.qtHiDpi", enableQtHighDpi_ );
    settings.setValue( "view.scaleFactorRounding", scaleFactorRounding_ );

    settings.setValue( "defaultView.searchAutoRefresh", searchAutoRefresh_ );
    settings.setValue( "defaultView.searchIgnoreCase", searchIgnoreCase_ );

    QList<QVariant> splitterSizes;
    std::transform( splitterSizes_.begin(), splitterSizes_.end(),
                    std::back_inserter( splitterSizes ),
                    []( auto s ) { return QVariant::fromValue( s ); } );

    settings.setValue( "defaultView.splitterSizes", splitterSizes );
}

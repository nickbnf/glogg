/*
 * Copyright (C) 2009, 2010, 2011, 2013 Nicolas Bonnefon and other contributors
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

#include <QtGui>

#include "optionsdialog.h"

#include "configuration.h"
#include "log.h"
#include "persistentinfo.h"
#include "versionchecker.h"

static const uint32_t POLL_INTERVAL_MIN = 10;
static const uint32_t POLL_INTERVAL_MAX = 3600000;

// Constructor
OptionsDialog::OptionsDialog( QWidget* parent )
    : QDialog( parent )
{
    setupUi( this );

    setupTabs();
    setupFontList();
    setupRegexp();

    // Validators
    QValidator* polling_interval_validator_
        = new QIntValidator( POLL_INTERVAL_MIN, POLL_INTERVAL_MAX, this );
    pollIntervalLineEdit->setValidator( polling_interval_validator_ );

    connect( buttonBox, &QDialogButtonBox::clicked, this, &OptionsDialog::onButtonBoxClicked );
    connect( fontFamilyBox, QOverload<const QString&>::of( &QComboBox::currentIndexChanged ), this,
             &OptionsDialog::updateFontSize );
    connect( incrementalCheckBox, &QCheckBox::toggled,
             [this]( auto ) { this->onIncrementalChanged(); } );
    connect( pollingCheckBox, &QCheckBox::toggled, [this]( auto ) { this->onPollingChanged(); } );
    connect( searchResultsCacheCheckBox, &QCheckBox::toggled,
             [this]( auto ) { this->onSearchResultsCacheChanged(); } );

    updateDialogFromConfig();

    setupIncremental();
    setupPolling();
    setupSearchResultsCache();
}

//
// Private functions
//

// Setups the tabs depending on the configuration
void OptionsDialog::setupTabs()
{
#ifndef GLOGG_SUPPORTS_POLLING
    pollBox->setVisible( false );
#endif

#ifndef GLOGG_SUPPORTS_VERSION_CHECKING
    versionCheckingBox->setVisible( false );
#endif

#ifndef Q_OS_WIN
    keepFileClosedCheckBox->setVisible( false );
#endif
}

// Populates the 'family' ComboBox
void OptionsDialog::setupFontList()
{
    QFontDatabase database;

    // We only show the fixed fonts
    const auto families = database.families();
    for ( const QString& str : families ) {
        if ( database.isFixedPitch( str ) )
            fontFamilyBox->addItem( str );
    }
}

// Populate the regexp ComboBoxes
void OptionsDialog::setupRegexp()
{
    QStringList regexpTypes;

    regexpTypes << tr( "Extended Regexp" ) << tr( "Wildcard" ) << tr( "Fixed Strings" );

    mainSearchBox->addItems( regexpTypes );
    quickFindSearchBox->addItems( regexpTypes );
}

// Enable/disable the QuickFind options depending on the state
// of the "incremental" checkbox.
void OptionsDialog::setupIncremental()
{
    if ( incrementalCheckBox->isChecked() ) {
        quickFindSearchBox->setCurrentIndex( getRegexpIndex( FixedString ) );
        quickFindSearchBox->setEnabled( false );
    }
    else {
        quickFindSearchBox->setEnabled( true );
    }
}

void OptionsDialog::setupPolling()
{
    pollIntervalLineEdit->setEnabled( pollingCheckBox->isChecked() );
}

void OptionsDialog::setupSearchResultsCache()
{
    searchCacheSpinBox->setEnabled( searchResultsCacheCheckBox->isChecked() );
}

// Convert a regexp type to its index in the list
int OptionsDialog::getRegexpIndex( SearchRegexpType syntax ) const
{
    int index;

    switch ( syntax ) {
    case Wildcard:
        index = 1;
        break;
    case FixedString:
        index = 2;
        break;
    default:
        index = 0;
        break;
    }

    return index;
}

// Convert the index of a regexp type to its type
SearchRegexpType OptionsDialog::getRegexpTypeFromIndex( int index ) const
{
    SearchRegexpType type;

    switch ( index ) {
    case 1:
        type = Wildcard;
        break;
    case 2:
        type = FixedString;
        break;
    default:
        type = ExtendedRegexp;
        break;
    }

    return type;
}

// Updates the dialog box using values in global Config()
void OptionsDialog::updateDialogFromConfig()
{
    const auto& config = Persistent<Configuration>( "settings" );

    // Main font
    QFontInfo fontInfo = QFontInfo( config.mainFont() );

    int familyIndex = fontFamilyBox->findText( fontInfo.family() );
    if ( familyIndex != -1 )
        fontFamilyBox->setCurrentIndex( familyIndex );

    int sizeIndex = fontSizeBox->findText( QString::number( fontInfo.pointSize() ) );
    if ( sizeIndex != -1 )
        fontSizeBox->setCurrentIndex( sizeIndex );

    // Regexp types
    mainSearchBox->setCurrentIndex( getRegexpIndex( config.mainRegexpType() ) );
    quickFindSearchBox->setCurrentIndex( getRegexpIndex( config.quickfindRegexpType() ) );

    incrementalCheckBox->setChecked( config.isQuickfindIncremental() );

    // Polling
    pollingCheckBox->setChecked( config.pollingEnabled() );
    pollIntervalLineEdit->setText( QString::number( config.pollIntervalMs() ) );

    // Last session
    loadLastSessionCheckBox->setChecked( config.loadLastSession() );

    // Perf
    parallelSearchCheckBox->setChecked( config.useParallelSearch() );
    searchResultsCacheCheckBox->setChecked( config.useSearchResultsCache() );
    searchCacheSpinBox->setValue( config.searchResultsCacheLines() );
    indexReadBufferSpinBox->setValue( config.indexReadBufferSizeMb() );
    searchReadBufferSpinBox->setValue( config.searchReadBufferSizeLines() );
    keepFileClosedCheckBox->setChecked( config.keepFileClosed() );

    // version checking
    const auto& versionChecking = Persistent<VersionCheckerConfig>( "versionChecker" );
    checkForNewVersionCheckBox->setChecked( versionChecking.versionCheckingEnabled() );
}

//
// Slots
//

void OptionsDialog::updateFontSize( const QString& fontFamily )
{
    QFontDatabase database;
    QString oldFontSize = fontSizeBox->currentText();
    auto sizes = database.pointSizes( fontFamily, "" );

    if (sizes.empty()) {
        sizes = QFontDatabase::standardSizes();
    }

    fontSizeBox->clear();
    for ( int size : sizes ) {
        fontSizeBox->addItem( QString::number( size ) );
    }
    // Now restore the size we had before
    int i = fontSizeBox->findText( oldFontSize );
    if ( i != -1 )
        fontSizeBox->setCurrentIndex( i );
}

void OptionsDialog::updateConfigFromDialog()
{
    auto& config = Persistent<Configuration>( "settings" );

    QFont font = QFont( fontFamilyBox->currentText(), ( fontSizeBox->currentText() ).toInt() );
    config.setMainFont( font );

    config.setMainRegexpType( getRegexpTypeFromIndex( mainSearchBox->currentIndex() ) );
    config.setQuickfindRegexpType( getRegexpTypeFromIndex( quickFindSearchBox->currentIndex() ) );
    config.setQuickfindIncremental( incrementalCheckBox->isChecked() );

    config.setPollingEnabled( pollingCheckBox->isChecked() );
    uint32_t poll_interval = pollIntervalLineEdit->text().toUInt();
    if ( poll_interval < POLL_INTERVAL_MIN )
        poll_interval = POLL_INTERVAL_MIN;
    else if ( poll_interval > POLL_INTERVAL_MAX )
        poll_interval = POLL_INTERVAL_MAX;

    config.setPollIntervalMs( poll_interval );

    config.setLoadLastSession( loadLastSessionCheckBox->isChecked() );

    config.setUseParallelSearch( parallelSearchCheckBox->isChecked() );
    config.setUseSearchResultsCache( searchResultsCacheCheckBox->isChecked() );
    config.setSearchResultsCacheLines( searchCacheSpinBox->value() );
    config.setIndexReadBufferSizeMb( indexReadBufferSpinBox->value() );
    config.setSearchReadBufferSizeLines( searchReadBufferSpinBox->value() );
    config.setKeepFileClosed( keepFileClosedCheckBox->isChecked() );

    // version checking
    auto& versionChecking = Persistent<VersionCheckerConfig>( "versionChecker" );
    versionChecking.setVersionCheckingEnabled( checkForNewVersionCheckBox->isChecked() );

    emit optionsChanged();
}

void OptionsDialog::onButtonBoxClicked( QAbstractButton* button )
{
    QDialogButtonBox::ButtonRole role = buttonBox->buttonRole( button );
    if ( ( role == QDialogButtonBox::AcceptRole ) || ( role == QDialogButtonBox::ApplyRole ) ) {
        updateConfigFromDialog();
    }

    if ( role == QDialogButtonBox::AcceptRole )
        accept();
    else if ( role == QDialogButtonBox::RejectRole )
        reject();
}

void OptionsDialog::onIncrementalChanged()
{
    setupIncremental();
}

void OptionsDialog::onPollingChanged()
{
    setupPolling();
}

void OptionsDialog::onSearchResultsCacheChanged()
{
    setupSearchResultsCache();
}

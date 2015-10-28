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

#include <QtGui>

#include "optionsdialog.h"

#include "log.h"
#include "persistentinfo.h"
#include "configuration.h"

static const uint32_t POLL_INTERVAL_MIN = 10;
static const uint32_t POLL_INTERVAL_MAX = 3600000;

// Constructor
OptionsDialog::OptionsDialog( QWidget* parent ) : QDialog(parent)
{
    setupUi( this );

    setupTabs();
    setupFontList();
    setupRegexp();

    // Validators
    QValidator* polling_interval_validator_ = new QIntValidator(
           POLL_INTERVAL_MIN, POLL_INTERVAL_MAX, this );
    pollIntervalLineEdit->setValidator( polling_interval_validator_ );

    connect(buttonBox, SIGNAL( clicked( QAbstractButton* ) ),
            this, SLOT( onButtonBoxClicked( QAbstractButton* ) ) );
    connect(fontFamilyBox, SIGNAL( currentIndexChanged(const QString& ) ),
            this, SLOT( updateFontSize( const QString& ) ));
    connect(incrementalCheckBox, SIGNAL( toggled( bool ) ),
            this, SLOT( onIncrementalChanged() ) );
    connect(pollingCheckBox, SIGNAL( toggled( bool ) ),
            this, SLOT( onPollingChanged() ) );

    updateDialogFromConfig();

    setupIncremental();
    setupPolling();
}

//
// Private functions
//

// Setups the tabs depending on the configuration
void OptionsDialog::setupTabs()
{
#ifndef GLOGG_SUPPORTS_POLLING
    tabWidget->removeTab( 1 );
#endif
}

// Populates the 'family' ComboBox
void OptionsDialog::setupFontList()
{
    QFontDatabase database;

    // We only show the fixed fonts
    foreach ( const QString &str, database.families() ) {
         if ( database.isFixedPitch( str ) )
             fontFamilyBox->addItem( str );
     }
}

// Populate the regexp ComboBoxes
void OptionsDialog::setupRegexp()
{
    QStringList regexpTypes;

    regexpTypes << tr("Extended Regexp")
        << tr("Wildcards") << tr("Fixed Strings");

    mainSearchBox->addItems( regexpTypes );
    quickFindSearchBox->addItems( regexpTypes );
}

// Enable/disable the QuickFind options depending on the state
// of the "incremental" checkbox.
void OptionsDialog::setupIncremental()
{
    if ( incrementalCheckBox->isChecked() ) {
        quickFindSearchBox->setCurrentIndex(
                getRegexpIndex( FixedString ) );
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

// Convert a regexp type to its index in the list
int OptionsDialog::getRegexpIndex( SearchRegexpType syntax ) const
{
    return static_cast<int>( syntax );
}

// Convert the index of a regexp type to its type
SearchRegexpType OptionsDialog::getRegexpTypeFromIndex( int index ) const
{
    return static_cast<SearchRegexpType>( index );;
}

// Updates the dialog box using values in global Config()
void OptionsDialog::updateDialogFromConfig()
{
    std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );

    // Main font
    QFontInfo fontInfo = QFontInfo( config->mainFont() );

    int familyIndex = fontFamilyBox->findText( fontInfo.family() );
    if ( familyIndex != -1 )
        fontFamilyBox->setCurrentIndex( familyIndex );

    int sizeIndex = fontSizeBox->findText( QString::number(fontInfo.pointSize()) );
    if ( sizeIndex != -1 )
        fontSizeBox->setCurrentIndex( sizeIndex );

    // Regexp types
    mainSearchBox->setCurrentIndex(
            getRegexpIndex( config->mainRegexpType() ) );
    quickFindSearchBox->setCurrentIndex(
            getRegexpIndex( config->quickfindRegexpType() ) );

    incrementalCheckBox->setChecked( config->isQuickfindIncremental() );

    // Polling
    pollingCheckBox->setChecked( config->pollingEnabled() );
    pollIntervalLineEdit->setText( QString::number( config->pollIntervalMs() ) );
}

//
// Slots
//

void OptionsDialog::updateFontSize(const QString& fontFamily)
{
    QFontDatabase database;
    QString oldFontSize = fontSizeBox->currentText();
    QList<int> sizes = database.pointSizes( fontFamily, "" );

    fontSizeBox->clear();
    foreach (int size, sizes) {
        fontSizeBox->addItem( QString::number(size) );
    }
    // Now restore the size we had before
    int i = fontSizeBox->findText(oldFontSize);
    if ( i != -1 )
        fontSizeBox->setCurrentIndex(i);
}

void OptionsDialog::updateConfigFromDialog()
{
    std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );

    QFont font = QFont(
            fontFamilyBox->currentText(),
            (fontSizeBox->currentText()).toInt() );
    config->setMainFont(font);

    config->setMainRegexpType(
            getRegexpTypeFromIndex( mainSearchBox->currentIndex() ) );
    config->setQuickfindRegexpType(
            getRegexpTypeFromIndex( quickFindSearchBox->currentIndex() ) );
    config->setQuickfindIncremental( incrementalCheckBox->isChecked() );

    config->setPollingEnabled( pollingCheckBox->isChecked() );
    uint32_t poll_interval = pollIntervalLineEdit->text().toUInt();
    if ( poll_interval < POLL_INTERVAL_MIN )
        poll_interval = POLL_INTERVAL_MIN;
    else if (poll_interval > POLL_INTERVAL_MAX )
        poll_interval = POLL_INTERVAL_MAX;

    config->setPollIntervalMs( poll_interval );

    emit optionsChanged();
}

void OptionsDialog::onButtonBoxClicked( QAbstractButton* button )
{
    QDialogButtonBox::ButtonRole role = buttonBox->buttonRole( button );
    if (   ( role == QDialogButtonBox::AcceptRole )
        || ( role == QDialogButtonBox::ApplyRole ) ) {
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

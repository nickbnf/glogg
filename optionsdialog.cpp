/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
 *
 * This file is part of LogCrawler.
 *
 * LogCrawler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LogCrawler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LogCrawler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>

#include "log.h"

#include "optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget* parent) : QDialog(parent)
{
    QGroupBox* fontBox  = new QGroupBox( tr("Font") );

    QLabel* sizeLabel   = new QLabel( tr("Size: ") );
    QLabel* familyLabel = new QLabel( tr("Family: ") );

    setupFontList();

    okButton     = new QPushButton( tr("OK") );
    cancelButton = new QPushButton( tr("Cancel") );
    applyButton  = new QPushButton( tr("Apply") );
    okButton->setDefault(true);

    connect(okButton, SIGNAL(clicked()), this, SLOT(updateConfigFromDialog()));
    connect(okButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(fontFamilyBox, SIGNAL(currentIndexChanged(const QString& )),
            this, SLOT(updateFontSize(const QString& )));

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(applyButton);

    QHBoxLayout* fontLayout = new QHBoxLayout;
    fontLayout->addWidget(familyLabel);
    fontLayout->addWidget(fontFamilyBox);
    fontLayout->addWidget(sizeLabel);
    fontLayout->addWidget(fontSizeBox);
    fontBox->setLayout(fontLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(fontBox);
    mainLayout->addLayout(buttonsLayout);

    setLayout(mainLayout);
    setWindowTitle( tr("Options") );
    setFixedHeight( sizeHint().height() );

    updateDialogFromConfig();
}

// Private functions
void OptionsDialog::setupFontList()
{
    QFontDatabase database;

    fontFamilyBox = new QComboBox();
    fontFamilyBox->addItems(database.families());

    fontSizeBox = new QComboBox();
}

void OptionsDialog::updateDialogFromConfig()
{
    // Main font
    QFont mainFont = Config().mainFont();

    int familyIndex = fontFamilyBox->findText( mainFont.family() );
    if ( familyIndex != -1 )
        fontFamilyBox->setCurrentIndex( familyIndex );

    int sizeIndex = fontSizeBox->findText( QString::number(mainFont.pointSize()) );
    if ( sizeIndex != -1 )
        fontSizeBox->setCurrentIndex( sizeIndex );
}

// Slots
void OptionsDialog::updateFontSize(const QString& text)
{
    QFontDatabase database;
    QString oldFontSize = fontSizeBox->currentText();
    QList<int> sizes = database.pointSizes( text, "" );

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
    LOG(logDEBUG) << "updateConfigFromDialog";

    QFont font = QFont(
            fontFamilyBox->currentText(),
            (fontSizeBox->currentText()).toInt() );
    Config().setMainFont(font);

    emit optionsChanged();
}

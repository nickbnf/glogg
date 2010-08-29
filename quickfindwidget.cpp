/*
 * Copyright (C) 2010 Nicolas Bonnefon and other contributors
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

#include "log.h"

#include "configuration.h"

#include "quickfindwidget.h"

QuickFindWidget::QuickFindWidget( QWidget* parent ) : QWidget( parent )
{
    // ui_.setupUi( this );
    // setFocusProxy(ui_.findEdit);
    // setProperty("topBorder", true);
    QHBoxLayout *layout = new QHBoxLayout( this );

    layout->setMargin( 0 );
    layout->setSpacing( 6 );

    closeButton_ = setupToolButton(
            QLatin1String(""), QLatin1String( ":/images/darkclosebutton.png" ) );
    layout->addWidget( closeButton_ );
    connect( closeButton_, SIGNAL( clicked() ), SLOT( hide() ) );

    editQuickFind_ = new QLineEdit( this );
    layout->addWidget( editQuickFind_ );
    editQuickFind_->setMinimumSize( QSize( 150, 0 ) );
    /*
    connect( editQuickFind_. SIGNAL( textChanged( QString ) ), this,
            SLOT( textChanged( QString ) ) );
    connect( editQuickFind_. SIGNAL( textChanged( QString ) ), this,
            SLOT( updateButtons() ) );
    */
    connect( editQuickFind_, SIGNAL( returnPressed() ),
            this, SLOT( returnHandler() ) );

    previousButton_ = setupToolButton( QLatin1String("Previous"),
            QLatin1String( ":/images/arrowup.png" ) );
    layout->addWidget( previousButton_ );
    connect( previousButton_, SIGNAL( clicked() ),
            this, SLOT( doSearchBackward() ) );

    nextButton_ = setupToolButton( QLatin1String("Next"),
            QLatin1String( ":/images/arrowdown.png" ) );
    layout->addWidget( nextButton_ );
    connect( nextButton_, SIGNAL( clicked() ),
            this, SLOT( doSearchForward() ) );

#if 0
    QSpacerItem* spacerItem = new QSpacerItem(20, 20, QSizePolicy::Expanding,
        QSizePolicy::Minimum);
    layout_->addItem(spacerItem);
#endif
    setMinimumWidth( minimumSizeHint().width() );
}

void QuickFindWidget::activate( QFDirection direction )
{
    direction_ = direction;
    QWidget::show();
    editQuickFind_->setFocus( Qt::ShortcutFocusReason );
}

//
// SLOTS
//

void QuickFindWidget::changeDisplayedPattern( const QString& newPattern )
{
    editQuickFind_->setText( newPattern );
}

void QuickFindWidget::doSearchForward()
{
    LOG(logDEBUG) << "QuickFindWidget::doSearchForward()";

    emit patternConfirmed( editQuickFind_->text() );
    emit searchForward();
}

void QuickFindWidget::doSearchBackward()
{
    LOG(logDEBUG) << "QuickFindWidget::doSearchBackward()";

    emit patternConfirmed( editQuickFind_->text() );
    emit searchBackward();
}

void QuickFindWidget::returnHandler()
{
    emit patternConfirmed( editQuickFind_->text() );
    // Close the widget
    this->hide();
    emit close();
    emit searchForward();
}

//
// Private functions
//
QToolButton* QuickFindWidget::setupToolButton(const QString &text, const QString &icon)
{
    QToolButton *toolButton = new QToolButton(this);

    toolButton->setText(text);
    toolButton->setAutoRaise(true);
    toolButton->setIcon(QIcon(icon));
    toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    return toolButton;
}

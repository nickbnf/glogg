/*
 * Copyright (C) 2009, 2010, 2011 Nicolas Bonnefon and other contributors
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

#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QTextCodec>
#include <QStringList>

#include "encodingselector.h"
#include "configuration.h"
#include "recentencodings.h"
#include "persistentinfo.h"

const QStringList& EncodingSelector::allEncodings()
{
    // Per-process lifetime
    static QStringList encodings;
    static bool initialized = false;

    if ( ! initialized ) {
        // Sort the available encodings using a map; display all aliases
        QMap<QString, QString> encodingsMap; // toLower(encoding) -> encoding
        foreach ( QByteArray encoding, QTextCodec::availableCodecs() ) {
            QString encodingStr( encoding );
            const QString& encodingStrLower = encodingStr.toLower();
            encodingsMap.insert( encodingStrLower, encodingStr );
        }
        encodings = encodingsMap.values();
        initialized = true;
    }
    return encodings;
}

EncodingSelector::EncodingSelector( Configuration* config,
                                    RecentEncodings* recentEncodings,
                                    QWidget* menuParent,
                                    QObject* parent )
    : QObject( parent )
    , menuParent_( menuParent )
    , config_( config )
    , recentEncodings_( recentEncodings )
    , actionGroup_( NULL )
    , actions_()
    , stickyActions_()
    , recentActions_()
    , defaultAction_( NULL )
    , stickyAndDefaultSeparator_( NULL )
    , recentSeparator_( NULL )
{
    createActions();
    createMenu();
}

QMenu* EncodingSelector::encodingsMenu() const
{
    return encodingsMenu_;
}

QString EncodingSelector::selectedEncoding() const
{
    QAction* action = actionGroup_->checkedAction();
    if ( action == NULL )
        return QString();
    return action->data().toString();
}

void EncodingSelector::applyConfiguration()
{
    changeDefaultEncoding();
}

void EncodingSelector::changeEncoding( QAction* action )
{
    makeRecent( action );
    QString encoding = action->data().toString();
    emit encodingChanged( encoding );
}

void EncodingSelector::createActions()
{
    const QStringList& encodings = EncodingSelector::allEncodings();

    actionGroup_ = new QActionGroup( this );
    connect( actionGroup_, SIGNAL( triggered( QAction* ) ),
             this, SLOT( changeEncoding( QAction* ) ) );

    foreach ( const QString& encoding, encodings ) {
        QAction* action = new QAction( encoding, this );
        action->setData( encoding ); // to not rely on display text
        action->setCheckable( true );
        action->setActionGroup( actionGroup_ );
        if ( encoding == config_->encoding() )
            action->setChecked( true );
        actions_.insert( encoding, action );
    }
}

void EncodingSelector::createMenu()
{
    // The encoding menu consists of foure parts:
    //  (1) submenu with list of all available encodings
    //  (2) section with "sticky" encodings:
    //         system, utf8, and user-selected default
    //  (3) item with the user-selected default encoding from options
    //  (4) section with (persisted) recently-used encodings

    encodingsMenu_ = new QMenu( tr("&Character Encoding"), menuParent_ );
    createMoreEncodingsSection();
    createStickySection();
    createDefaultSection();
    createRecentSection();
}

void EncodingSelector::createMoreEncodingsSection()
{
    moreEncodingsMenu_ = encodingsMenu_->addMenu( tr("&More encodings") );
    const QStringList& encodings = EncodingSelector::allEncodings();
    foreach ( const QString& encoding, encodings ) {
        moreEncodingsMenu_->addAction( actions_[ encoding ] );
    }
}

void EncodingSelector::createStickySection()
{
    // The sticky actions might coincide, and we must not add
    // a sticky action more than once since that causes re-ordering,
    // so we check membership before adding each one.

    // Locale
    QString localeEncoding = QTextCodec::codecForLocale()->name();
    if ( actions_.contains( localeEncoding ) ) {
        QAction* action = actions_[ localeEncoding ];
        if ( ! stickyActions_.contains( action ) )
            stickyActions_.append( action );
    }

    // UTF-8
    const char utf8Encoding[] = "UTF-8";
    if ( actions_.contains( utf8Encoding ) ) {
        QAction* action = actions_[ utf8Encoding ];
        if ( ! stickyActions_.contains( action ) )
            stickyActions_.append( action );
    }

    // Push from model to view
    if ( ! stickyActions_.empty() ) {
        stickyAndDefaultSeparator_ = encodingsMenu_->addSeparator();
        foreach ( QAction* action, stickyActions_ )
            encodingsMenu_->addAction( action );
    }
}

void EncodingSelector::createDefaultSection()
{
    // User-persisted default
    if ( actions_.contains( config_->encoding() ) ) {
        QAction* action = actions_[ config_->encoding() ];
        if ( ! stickyActions_.contains( action ) )
            defaultAction_ = action;
    }

    // Push from model to view
    if ( defaultAction_ != NULL ) {
        if ( stickyAndDefaultSeparator_ == NULL )
            stickyAndDefaultSeparator_ = encodingsMenu_->addSeparator();
        encodingsMenu_->addAction( defaultAction_ );
    }
}

void EncodingSelector::createRecentSection()
{
    // Resolve encoding names into actions and add them to model
    foreach ( QString encoding, recentEncodings_->recentEncodings() ) {
        if (  actions_.contains( encoding )) {
            QAction* action = actions_[ encoding ];
            // Invariant: recentActions and {stickyActions,default} are disjoint
            if ( ! stickyActions_.contains( action ) &&
                 action != defaultAction_ ) {
                recentActions_.append( action );
            }
        }
    }

    // Push from model to view
    if ( ! recentActions_.empty() ) {
        recentSeparator_ = encodingsMenu_->addSeparator();
        foreach ( QAction* action, recentActions_ ) {
            encodingsMenu_->addAction( action );
        }
    }
}

void EncodingSelector::changeDefaultEncoding()
{
    // This method updates the menu item corresponding to persisted
    // user-selected default encoding. We assume that the default goes right
    // before the recents separator.

    // Get the new default
    QAction* defaultAction = NULL;
    if ( actions_.contains( config_->encoding() ) )
        defaultAction = actions_[ config_->encoding() ];

    // Check if there was any change at all
    if ( defaultAction == defaultAction_ )
        return;

    // Update the model
    QAction* oldDefaultAction = defaultAction_;
    int defaultIndexInRecents = -1;
    if ( defaultAction != NULL ) {
        if ( ! stickyActions_.contains( defaultAction ) ) {
            // Remove the new one in case it appears in recents
            // Invariant: recentActions and {stickyActions,default} are disjoint
            if ( recentActions_.contains( defaultAction ) ) {
                defaultIndexInRecents = recentActions_.indexOf( defaultAction );
                recentActions_.removeAt( defaultIndexInRecents );
            }
            defaultAction_ = defaultAction;
        } else {
            // It's in 'sticky' section, don't show it in 'default' section
            defaultAction_ = NULL;
        }
    } else {
        // New default is null, so clear the 'default' section
        defaultAction_ = NULL;
    }

    // Push changes in model to view

    // Add separator if needed and if it is not there
    if ( ! stickyActions_.empty() || defaultAction_ != NULL ) {
        if ( stickyAndDefaultSeparator_ == NULL )
            stickyAndDefaultSeparator_ = encodingsMenu_->addSeparator();
    } else {
        if ( stickyAndDefaultSeparator_ != NULL )
            encodingsMenu_->removeAction( stickyAndDefaultSeparator_ );
    }

    if ( oldDefaultAction != NULL )
        encodingsMenu_->removeAction( oldDefaultAction );

    if ( defaultIndexInRecents >= 0 )
        encodingsMenu_->removeAction( defaultAction_ );

    if ( defaultAction_ != NULL ) {
        // Invariant: recents list is empty => recentSeparator is null => append;
        //            otherwise insert before it
        encodingsMenu_->insertAction( recentSeparator_, defaultAction_ );
    }

    // Add old default to recents section (convenient behavior)
    if ( oldDefaultAction != NULL )
        makeRecent( oldDefaultAction );
}

void EncodingSelector::makeRecent( QAction* action )
{
    // Invariant: recentActions and {stickyActions,default} are disjoint
    // So, if it's in either sticky or default, then there's nothing to do
    if ( stickyActions_.contains( action ) || action == defaultAction_ )
        return;

    // First remove from recents if it is already there, to reorder
    int actionIndex = -1;
    if ( recentActions_.contains( action ) ) {
        actionIndex = recentActions_.indexOf( action );
        recentActions_.removeAt( actionIndex );
    }

    // If recents are full: remove oldest
    // Invariant: recentActions_.size() <= recentEncodings_->max()
    QAction* oldestAction = NULL;
    if ( recentActions_.size() > 0 &&
         recentActions_.size() == recentEncodings_->max() ) {
        oldestAction = recentActions_.last();
        recentActions_.removeLast();
    }

    // Add to model
    recentActions_.prepend( action );

    // Remember in persistable collection
    QString encoding = action->data().toString();
    recordRecent( encoding );

    // Invariant: recentActions_.size() >= 1 (we just appended)

    // Push the above model changes to view

    // If we added the first one: add the separator
    if ( recentSeparator_ == NULL ) // will be the case on first add
        recentSeparator_ = encodingsMenu_->addSeparator();

    if ( actionIndex >= 0 )
        encodingsMenu_->removeAction( action );
    if ( oldestAction != NULL )
        encodingsMenu_->removeAction( oldestAction );

    // Insert before the first action (our action is at index 0)
    QAction* followingAction = recentActions_.size() > 1 ?
        recentActions_.at( 1 ) : NULL;
    encodingsMenu_->insertAction( followingAction, action );
}

void EncodingSelector::recordRecent( const QString& encoding )
{
    // Push the encoding into the list of recents
    // (reload the list first in case another glogg changed it)
    // TODO: Encapsulate load-modify-save logic into persistables
    GetPersistentInfo().retrieve( "recentEncodings" );
    recentEncodings_->addRecent( encoding );
    GetPersistentInfo().save( "recentEncodings" );
}

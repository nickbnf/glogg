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
                                    RecentEncodings* recentEncodingsStore,
                                    QWidget* menuParent,
                                    QObject* parent )
    : QObject( parent )
    , menuParent_( menuParent )
    , config_( config )
    , recentEncodingsStore_( recentEncodingsStore )
    , actionGroup_( NULL )
    , stickyEncodings_()
    , recentEncodings_()
    , defaultEncoding_()
    , stickyAndDefaultSeparator_( NULL )
    , recentSeparator_( NULL )
{
    // We manage the checkmarks manually, since there are multiple
    // actions for the same encoding (in different menu sections).
    // NOTE: Unity/DBus doesn't like multiple ptrs to same action object in a
    // menu.
    actionGroup_ = new QActionGroup( this );
    actionGroup_->setExclusive( false );
    connect( actionGroup_, SIGNAL( triggered( QAction* ) ),
             this, SLOT( changeEncoding( QAction* ) ) );

    createMenu();

    selectedEncoding_ = config_->encoding();
    updateCheckmarks();
}

QMenu* EncodingSelector::encodingsMenu() const
{
    return encodingsMenu_;
}

QString EncodingSelector::selectedEncoding() const
{
    return selectedEncoding_;
}

void EncodingSelector::applyConfiguration()
{
    changeDefaultEncoding( config_->encoding() );
}

void EncodingSelector::changeEncoding( QAction* action )
{
    selectedEncoding_ = action->data().toString();
    makeRecent( selectedEncoding_ );
    updateCheckmarks();
    emit encodingChanged( selectedEncoding_ );
}

QAction* EncodingSelector::createAction( const QString& encoding,
                                         QWidget* parent )
{
    QAction* action = new QAction( encoding, parent );
    action->setData( encoding ); // to not rely on display text
    action->setCheckable( true );
    action->setActionGroup( actionGroup_ );
    return action;
}

void EncodingSelector::destroyAction( QAction *action )
{
    actionGroup_->removeAction( action );
    delete action;
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
        QAction* action = createAction( encoding, moreEncodingsMenu_ );
        moreEncodingsMenu_->addAction( action );
    }
}

void EncodingSelector::createStickySection()
{
    // The sticky actions might coincide, and we must not add
    // a sticky action more than once since that causes re-ordering,
    // so we check membership before adding each one.

    const QStringList& allEncodings = EncodingSelector::allEncodings();

    // Locale
    QString localeEncoding = QTextCodec::codecForLocale()->name();
    if ( allEncodings.contains( localeEncoding ) &&
         ! stickyEncodings_.contains( localeEncoding ) ) {
        stickyEncodings_.append( localeEncoding );
    }

    // UTF-8
    const char utf8Encoding[] = "UTF-8";
    if ( allEncodings.contains( utf8Encoding ) &&
         ! stickyEncodings_.contains( utf8Encoding ) ) {
        stickyEncodings_.append( utf8Encoding );
    }

    // Push from model to view
    if ( ! stickyEncodings_.empty() ) {
        stickyAndDefaultSeparator_ = encodingsMenu_->addSeparator();
        foreach ( const QString& encoding, stickyEncodings_ ) {
            QAction* action = createAction( encoding, encodingsMenu_ );
            encodingsMenu_->addAction( action );
        }
    }
}

void EncodingSelector::createDefaultSection()
{
    const QStringList& allEncodings = EncodingSelector::allEncodings();

    // User-persisted default
    const QString& defaultEncoding = config_->encoding();
    if ( allEncodings.contains( defaultEncoding ) &&
         ! stickyEncodings_.contains( defaultEncoding ) ) {
        defaultEncoding_ = defaultEncoding;
    }

    // Push from model to view
    if ( defaultEncoding_ != "" ) {
        if ( stickyAndDefaultSeparator_ == NULL )
            stickyAndDefaultSeparator_ = encodingsMenu_->addSeparator();

        QAction* action = createAction( defaultEncoding_, encodingsMenu_ );
        encodingsMenu_->addAction( action );
    }
}

void EncodingSelector::createRecentSection()
{
    const QStringList& allEncodings = EncodingSelector::allEncodings();

    // Resolve encoding names into actions and add them to model
    foreach ( QString encoding, recentEncodingsStore_->recentEncodings() ) {
        if (  allEncodings.contains( encoding )) {
            // Invariant: recentActions and {stickyActions,default} are disjoint
            if ( ! stickyEncodings_.contains( encoding ) &&
                 encoding != defaultEncoding_ ) {
                recentEncodings_.append( encoding );
            }
        }
    }

    // Push from model to view
    if ( ! recentEncodings_.empty() ) {
        recentSeparator_ = encodingsMenu_->addSeparator();
        foreach ( const QString& encoding, recentEncodings_ ) {
            QAction* action = createAction( encoding, encodingsMenu_ );
            encodingsMenu_->addAction( action );
        }
    }
}

void EncodingSelector::updateCheckmarks()
{
    const QMenu* sections[] = {
        moreEncodingsMenu_,
        encodingsMenu_,
        NULL
    };
    for (int i = 0; sections[i] != NULL; ++i) {
        foreach ( QAction* action, sections[i]->actions() ) {
            if ( action->data().toString() == selectedEncoding_ )
                action->setChecked( true );
            else
                action->setChecked( false);
        }
    }
}

int EncodingSelector::recentIndexToMenuIndex( int recentIndex )
{
    // List of recent items doesn't start at zero in the menu, so calculate
    return 1 + // more encodings
           ( stickyAndDefaultSeparator_ != NULL ? 1 : 0 ) +
           stickyEncodings_.size() +
           ( defaultEncoding_ != "" ? 1 : 0 ) +
           ( recentSeparator_ != NULL ? 1 : 0 ) +
           recentIndex;
}

int EncodingSelector::defaultEncodingMenuIndex()
{
    return 1 + // more encodings
           ( stickyAndDefaultSeparator_ != NULL ? 1 : 0 ) +
           stickyEncodings_.length();
}

void EncodingSelector::changeDefaultEncoding( const QString& encoding )
{
    // This method updates the menu item corresponding to persisted
    // user-selected default encoding. We assume that the default goes right
    // before the recents separator.

    const QStringList& allEncodings = EncodingSelector::allEncodings();

    // The new default
    QString defaultEncoding = "";
    if ( allEncodings.contains( encoding ) )
        defaultEncoding = encoding;

    // Check if there was any change at all
    if ( defaultEncoding == defaultEncoding_ )
        return;

    // Update the model
    QString oldDefaultEncoding = defaultEncoding_;
    int defaultIndexInRecents = -1;
    if ( defaultEncoding != "" ) {
        if ( ! stickyEncodings_.contains( defaultEncoding ) ) {
            // Remove the new one in case it appears in recents
            // Invariant: recentActions and {stickyActions,default} are disjoint
            if ( recentEncodings_.contains( defaultEncoding ) ) {
                defaultIndexInRecents = recentEncodings_.indexOf( defaultEncoding );
                recentEncodings_.removeAt( defaultIndexInRecents );
            }
            defaultEncoding_ = defaultEncoding;
        } else {
            // It's in 'sticky' section, don't show it in 'default' section
            defaultEncoding_ = "";
        }
    } else {
        // New default is null, so clear the 'default' section
        defaultEncoding_ = "";
    }

    // Push changes in model to view

    // Add separator if needed and if it is not there
    if ( ! stickyEncodings_.empty() || defaultEncoding_ != "" ) {
        if ( stickyAndDefaultSeparator_ == NULL )
            stickyAndDefaultSeparator_ = encodingsMenu_->addSeparator();
    } else {
        if ( stickyAndDefaultSeparator_ != NULL )
            encodingsMenu_->removeAction( stickyAndDefaultSeparator_ );
    }

    if ( oldDefaultEncoding != "") {
        // The default menu item is the one after 'sticky' section
        const QList<QAction*>& actions = encodingsMenu_->actions();
        QAction* action = actions.at( defaultEncodingMenuIndex() );
        encodingsMenu_->removeAction( action );
        destroyAction( action );
    }

    if ( defaultIndexInRecents >= 0 ) {
        const QList<QAction*>& actions = encodingsMenu_->actions();
        QAction* action =
            // -1 because there is no default enc item (invariant at this point)
            actions.at( recentIndexToMenuIndex( defaultIndexInRecents ) - 1 );
        encodingsMenu_->removeAction( action );
        destroyAction( action );
    }

    if ( defaultEncoding_ != "" ) {
        // Invariant: recents list is empty => recentSeparator is null => append;
        //            otherwise insert before it
        QAction* action = createAction( defaultEncoding, encodingsMenu_ );
        encodingsMenu_->insertAction( recentSeparator_, action );
    }

    // Add old default to recents section (convenient behavior)
    if ( oldDefaultEncoding != "" )
        makeRecent( oldDefaultEncoding );

    // Actions might have been added to menus, so need to update checkmarks
    updateCheckmarks();
}

void EncodingSelector::makeRecent( const QString& encoding )
{
    // Invariant: recentActions and {stickyActions,default} are disjoint
    // So, if it's in either sticky or default, then there's nothing to do
    if ( stickyEncodings_.contains( encoding ) || encoding == defaultEncoding_ )
        return;

    // First remove from recents if it is already there, to reorder
    int encodingIndexInRecents = -1;
    if ( recentEncodings_.contains( encoding ) ) {
        encodingIndexInRecents = recentEncodings_.indexOf( encoding );
        recentEncodings_.removeAt( encodingIndexInRecents );
    }

    // If recents are full: remove oldest
    // Invariant: recentEncodings_.size() <= recentEncodings_->max()
    int oldestActionIndexInRecents = -1;
    if ( recentEncodings_.size() > 0 &&
         recentEncodings_.size() == recentEncodingsStore_->max() ) {
        oldestActionIndexInRecents = recentEncodings_.size() - 1;
        recentEncodings_.removeLast();
    }

    // Add to model
    recentEncodings_.prepend( encoding );

    // Remember in persistable collection
    recordRecent( encoding );

    // Invariant: recentEncodings_.size() >= 1 (we just appended)

    // Push the above model changes to view

    // If we added the first one: add the separator
    if ( recentSeparator_ == NULL ) // will be the case on first add
        recentSeparator_ = encodingsMenu_->addSeparator();

    if ( encodingIndexInRecents >= 0 ) {
        const QList<QAction*>& actions = encodingsMenu_->actions();
        QAction* action =
            actions.at( recentIndexToMenuIndex( encodingIndexInRecents ) );
        encodingsMenu_->removeAction( action );
        destroyAction( action );
    }
    // TODO: adjust the menu item index corresponding to
    // oldestActionIndexInRecents based on encodingIndexInRecents
    if ( oldestActionIndexInRecents >= 0 ) {
        const QList<QAction*>& actions = encodingsMenu_->actions();
        QAction* action =
            actions.at( recentIndexToMenuIndex( oldestActionIndexInRecents ) );
        encodingsMenu_->removeAction( action );
        destroyAction( action );
    }

    // Insert before the first action (our action is at index 0)
    QAction* recentAction = createAction( encoding, encodingsMenu_ );
    const QList<QAction*>& actions = encodingsMenu_->actions();
    // First action in the recent list before the new recent was prepended
    QAction* followingAction = recentEncodings_.size() <= 1 ?  NULL :
        actions.at( recentIndexToMenuIndex( 0 ) );
    encodingsMenu_->insertAction( followingAction, recentAction );
}

void EncodingSelector::recordRecent( const QString& encoding )
{
    // Push the encoding into the list of recents
    // (reload the list first in case another glogg changed it)
    // TODO: Encapsulate load-modify-save logic into persistables
    GetPersistentInfo().retrieve( "recentEncodings" );
    recentEncodingsStore_->addRecent( encoding );
    GetPersistentInfo().save( "recentEncodings" );
}

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

#ifndef ENCODINGSELECTOR_H
#define ENCODINGSELECTOR_H

#include <QString>
#include <QList>
#include <QHash>

class QObject;
class QWidget;
class QMenu;
class QAction;
class QActionGroup;
class QStringList;

class Configuration;
class RecentEncodings;

// Provides a menu for choosing encodings
//
// The menu is composed of four sections: list of all known encodings,
// a list of a few permanently-visible encodings for convenience, the
// user-specified default encoding from options, and a list
// of recently used encodings. Note that there is no separator between
// sticky and default sections to minimize number of separators.
//
// Implementation notes: the logic is implemented by operating on the models
// corresponding to the different sections of the menu. When the models are
// modified, the modifications are pushed to the view (the actual menu). This
// means that there should never be the need to read from the menu.
class EncodingSelector : public QObject
{
    Q_OBJECT

  public:
    // List of all known encodings (sorted alphabetically, aliases kept)
    static const QStringList& allEncodings();

  public:
    EncodingSelector( Configuration* config, RecentEncodings* recentEncodings,
                      QWidget* menuParent, QObject* parent = 0 );

  private:
    EncodingSelector( const EncodingSelector& );
    EncodingSelector& operator=( const EncodingSelector& );

  public:

    // The menu used to choose encoding
    QMenu* encodingsMenu() const;

    // Currently selected encoding
    QString selectedEncoding() const;

  public slots:
    void applyConfiguration();

  signals:
    void encodingChanged( const QString& encoding );

  private slots:
    void changeEncoding( QAction* encodingAction );

  private:
    void createActions();
    void createMenu();
    void createMoreEncodingsSection();
    void createStickySection();
    void createDefaultSection();
    void createRecentSection();

    // Updates the default-encoding item in the menu
    void changeDefaultEncoding();

    // Perform necessary steps to mark the encoding as recently-used
    void makeRecent( QAction* action );

    // Remember encoding as recently-used by adding to persistable collection
    void recordRecent( const QString& encoding );

    // This widget will be used as the menu parent
    QWidget* menuParent_;

    Configuration* config_;
    RecentEncodings* recentEncodings_;

    QMenu* encodingsMenu_;
    QMenu* moreEncodingsMenu_;

    // All encoding actions are in this group
    QActionGroup* actionGroup_;

    // Stores the action for each encoding known to the selector
    // Maps: encoding name -> action
    QHash<QString, QAction*> actions_;

    // Model collection for the menu section with permanently-visible encodings
    // Invariant: disjoint with recentActions_
    QList<QAction*> stickyActions_;

    // Model collection for the menu section with recently used encodings
    // (ordered newest to oldest)
    // Invariant: disjoint with {stickyActions,default}
    QList<QAction*> recentActions_;

    // Model menu item for the default encoding set in options
    // Invariant: not in {stickyActions,recentActions}
    QAction* defaultAction_;

    // Separator before the sticky+default sections and the recents section
    // We don't explicitly separate sticky+default, so they share this separator
    // Invariant: not null if {stickyActions,default} is not empty
    QAction* stickyAndDefaultSeparator_;

    // Separator before the sticky section
    // Invariant: not null if recentActions_ is not empty
    QAction* recentSeparator_;

};

#endif

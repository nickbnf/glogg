/*
 * Copyright (C) 2009, 2010, 2011, 2013, 2014 Nicolas Bonnefon and other contributors
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <QMainWindow>
#include <array>

#include "session.h"
#include "crawlerwidget.h"
#include "infoline.h"
#include "signalmux.h"
#include "tabbedcrawlerwidget.h"
#include "quickfindwidget.h"
#include "quickfindmux.h"
#ifdef GLOGG_SUPPORTS_VERSION_CHECKING
#include "versionchecker.h"
#endif

class QAction;
class QActionGroup;
class Session;
class RecentFiles;
class MenuActionToolTipBehavior;
class ExternalCommunicator;

// Main window of the application, creates menus, toolbar and
// the CrawlerWidget
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    // Constructor
    // The ownership of the session is transferred to us
    MainWindow( std::unique_ptr<Session> session,
            std::shared_ptr<ExternalCommunicator> external_communicator );

    // Re-install the geometry stored in config file
    // (should be done before 'Widget::show()')
    void reloadGeometry();
    // Re-load the files from the previous session
    void reloadSession();
    // Loads the initial file (parameter passed or from config file)
    void loadInitialFile( QString fileName );
    // Starts the lower priority activities the MW controls such as
    // version checking etc...
    void startBackgroundTasks();

  protected:
    void closeEvent( QCloseEvent* event );
    // Drag and drop support
    void dragEnterEvent( QDragEnterEvent* event );
    void dropEvent( QDropEvent* event );
    void keyPressEvent( QKeyEvent* keyEvent );

  private slots:
    void open();
    void openRecentFile( QAction* recentFileAction );
    void closeTab();
    void closeAll();
    void selectAll();
    void copy();
    void find();
    void filters();
    void options();
    void about();
    void aboutQt();
    void encodingChanged( QAction* action );

    // Change the view settings
    void toggleOverviewVisibility( bool isVisible );
    void toggleMainLineNumbersVisibility( bool isVisible );
    void toggleFilteredLineNumbersVisibility( bool isVisible );

    // Change the follow mode checkbox and send the followSet signal down
    void changeFollowMode( bool follow );

    // Update the line number displayed in the status bar.
    // Must be passed as the internal (starts at 0) line number.
    void lineNumberHandler(LineNumber line );

    // Instructs the widget to update the loading progress gauge
    void updateLoadingProgress( int progress );
    // Instructs the widget to display the 'normal' status bar,
    // without the progress gauge and with file info
    // or an error recovery when loading is finished
    void handleLoadingFinished( LoadingStatus status );

    // Save the new state as default setting when a crawler
    // is changing their view options.
    void handleSearchRefreshChanged( int state );
    void handleIgnoreCaseChanged( int state );

    // Close the tab with the passed index
    void closeTab( int index );
    // Setup the tab with current index for view
    void currentTabChanged( int index );

    // Instructs the widget to change the pattern in the QuickFind widget
    // and confirm it.
    void changeQFPattern( const QString& newPattern );

    // Load a file in a new tab (non-interactive)
    // (for use from e.g. IPC)
    void loadFileNonInteractive( const QString& file_name );

    // Notify the user a new version is available
    void newVersionNotification( const QString& new_version );

  signals:
    // Is emitted when new settings must be used
    void optionsChanged();
    // Is emitted when the 'follow' option is enabled/disabled
    void followSet( bool checked );
    // Is emitted before the QuickFind box is activated,
    // to allow crawlers to get search in the right view.
    void enteringQuickFind();
    // Emitted when the quickfind bar is closed.
    void exitingQuickFind();

  private:
    void createActions();
    void createMenus();
    void createContextMenu();
    void createToolBars();
    void createStatusBar();
    void createRecentFileToolTipTimer();
    void readSettings();
    void writeSettings();
    bool loadFile( const QString& fileName );
    void updateTitleBar( const QString& file_name );
    void updateRecentFileActions();
    QString strippedName( const QString& fullFileName ) const;
    CrawlerWidget* currentCrawlerWidget() const;
    void displayQuickFindBar( QuickFindMux::QFDirection direction );
    void updateMenuBarFromDocument( const CrawlerWidget* crawler );
    void updateInfoLine();

    std::unique_ptr<Session> session_;
    std::shared_ptr<ExternalCommunicator> externalCommunicator_;
    std::shared_ptr<RecentFiles> recentFiles_;
    QString loadingFileName;

    // Encoding
    struct EncodingList {
        const char* name;
    };

    static const EncodingList encoding_list[];

    enum { MaxRecentFiles = 5 };
    std::array<QAction*, MaxRecentFiles> recentFileActions;
    std::array<MenuActionToolTipBehavior*, MaxRecentFiles> recentFileActionBehaviors;

    QAction *separatorAction;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *toolsMenu;
    QMenu *encodingMenu;
    QMenu *helpMenu;

    InfoLine *infoLine;
    QLabel* lineNbField;
    QToolBar *toolBar;

    QAction *openAction;
    QAction *closeAction;
    QAction *closeAllAction;
    QAction *exitAction;
    QAction *copyAction;
    QAction *selectAllAction;
    QAction *findAction;
    QAction *overviewVisibleAction;
    QAction *lineNumbersVisibleInMainAction;
    QAction *lineNumbersVisibleInFilteredAction;
    QAction *followAction;
    QAction *reloadAction;
    QAction *stopAction;
    QAction *filtersAction;
    QAction *optionsAction;
    QAction *aboutAction;
    QAction *aboutQtAction;
    QActionGroup *encodingGroup;
    std::array<QAction*, CrawlerWidget::ENCODING_MAX> encodingAction;

    QIcon mainIcon_;

    // Multiplex signals to any of the CrawlerWidgets
    SignalMux signalMux_;

    // QuickFind widget
    QuickFindWidget quickFindWidget_;

    // Multiplex signals to/from the QuickFindWidget
    QuickFindMux quickFindMux_;

    // The main widget
    TabbedCrawlerWidget mainTabWidget_;

    // Version checker
#ifdef GLOGG_SUPPORTS_VERSION_CHECKING
    VersionChecker versionChecker_;
#endif
};

#endif

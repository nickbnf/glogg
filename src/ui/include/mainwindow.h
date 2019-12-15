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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <QMainWindow>
#include <QSystemTrayIcon>

#include <array>

#include "session.h"
#include "crawlerwidget.h"
#include "pathline.h"
#include "signalmux.h"
#include "tabbedcrawlerwidget.h"
#include "quickfindwidget.h"
#include "quickfindmux.h"
#include "tabbedscratchpad.h"

class QAction;
class QActionGroup;
class Session;
class RecentFiles;
class MenuActionToolTipBehavior;

// Main window of the application, creates menus, toolbar and
// the CrawlerWidget
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow( WindowSession session );

    // Re-install the geometry stored in config file
    // (should be done before 'Widget::show()')
    void reloadGeometry();
    // Re-load the files from the previous session
    void reloadSession();
    // Loads the initial file (parameter passed or from config file)
    void loadInitialFile( QString fileName, bool followFile );

  public slots:
    // Load a file in a new tab (non-interactive)
    // (for use from e.g. IPC)
    void loadFileNonInteractive( const QString& file_name );

  protected:
    void closeEvent( QCloseEvent* event ) override;
    void changeEvent( QEvent* event ) override;

    // Drag and drop support
    void dragEnterEvent( QDragEnterEvent* event ) override;
    void dropEvent( QDropEvent* event ) override;
    void keyPressEvent( QKeyEvent* keyEvent ) override;

    bool event(QEvent *event) override;

  private slots:
    void open();
    void openRecentFile( QAction* recentFileAction );
    void closeTab();
    void closeAll();
    void selectAll();
    void copy();
    void find();
    void clearLog();
    void copyFullPath();
    void openContainingFolder();
    void openInEditor();
    void openClipboard();
    void highlighters();
    void options();
    void about();
    void aboutQt();
    void showScratchPad();
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
    void handleSearchRefreshChanged( bool isRefreshing );
    void handleMatchCaseChanged( bool matchCase );

    // Close the tab with the passed index
    void closeTab( int index );
    // Setup the tab with current index for view
    void currentTabChanged( int index );

    // Instructs the widget to change the pattern in the QuickFind widget
    // and confirm it.
    void changeQFPattern( const QString& newPattern );

    void onClipboardDataChanged();
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

    void newWindow();
    void windowActivated();
    void windowClosed();
    void exitRequested();

  private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createTrayIcon();
    void readSettings();
    void writeSettings();
    bool loadFile( const QString& fileName, bool followFile = false );
    void updateTitleBar( const QString& file_name );
    void updateRecentFileActions();
    QString strippedName( const QString& fullFileName ) const;
    CrawlerWidget* currentCrawlerWidget() const;
    void displayQuickFindBar( QuickFindMux::QFDirection direction );
    void updateMenuBarFromDocument( const CrawlerWidget* crawler );
    void updateInfoLine();
    void showInfoLabels( bool show );

    WindowSession session_;
    QString loadingFileName;

    std::array<QString, static_cast<size_t>(CrawlerWidget::Encoding::MAX)> encodingNames_ = {
        "&Auto",
        "System",
        "ASCII",
        "UTF-8",
        "CP1251",
        "UTF-16LE",
        "UTF-16BE",
        "UTF-32LE",
        "UTF-32BE",
        "Big5",
        "GB18030",
        "Shift-JIS",
        "KOI8-R"
    };

    enum { MaxRecentFiles = 5 };
    std::array<QAction*, MaxRecentFiles> recentFileActions;
    std::array<MenuActionToolTipBehavior*, MaxRecentFiles> recentFileActionBehaviors;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *toolsMenu;
    QMenu *encodingMenu;
    QMenu *helpMenu;

    PathLine *infoLine;
    QLabel* lineNbField;
    QLabel* sizeField;
    QLabel* dateField;
    QLabel* encodingField;
    std::vector<QAction*> infoToolbarSeparators;
    QToolBar *toolBar;

    QAction *newWindowAction;
    QAction *openAction;
    QAction *closeAction;
    QAction *closeAllAction;
    QAction *exitAction;
    QAction *copyAction;
    QAction *selectAllAction;
    QAction *findAction;
    QAction *clearLogAction;
    QAction *copyPathToClipboardAction;
    QAction *openContainingFolderAction;
    QAction *openInEditorAction;
    QAction *openClipboardAction;
    QAction *overviewVisibleAction;
    QAction *lineNumbersVisibleInMainAction;
    QAction *lineNumbersVisibleInFilteredAction;
    QAction *followAction;
    QAction *reloadAction;
    QAction *stopAction;
    QAction *highlightersAction;
    QAction *optionsAction;
    QAction *showScratchPadAction;
    QAction *aboutAction;
    QAction *aboutQtAction;
    QActionGroup *encodingGroup;
    std::array<QAction*, static_cast<int>(CrawlerWidget::Encoding::MAX)> encodingAction;

    QSystemTrayIcon *trayIcon_;

    QIcon mainIcon_;

    // Multiplex signals to any of the CrawlerWidgets
    SignalMux signalMux_;

    // QuickFind widget
    QuickFindWidget quickFindWidget_;

    // Multiplex signals to/from the QuickFindWidget
    QuickFindMux quickFindMux_;

    // The main widget
    TabbedCrawlerWidget mainTabWidget_;

    TabbedScratchPad scratchPad_;

    bool isMaximized_ = false;
    bool isCloseFromTray_ = false;
};

#endif

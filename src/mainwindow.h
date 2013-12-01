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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <QMainWindow>

#include "session.h"
#include "crawlerwidget.h"
#include "infoline.h"
#include "signalmux.h"

class QAction;
class Session;
class RecentFiles;
class MenuActionToolTipBehavior;

// Main window of the application, creates menus, toolbar and
// the CrawlerWidget
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    // Constructor
    // The ownership of the session is transferred to us
    MainWindow( std::unique_ptr<Session> session );

    // Loads the initial file (parameter passed or from config file)
    void loadInitialFile( QString fileName );

  protected:
    void closeEvent( QCloseEvent* event );
    // Drag and drop support
    void dragEnterEvent( QDragEnterEvent* event );
    void dropEvent( QDropEvent* event );

  private slots:
    void open();
    void openRecentFile();
    void selectAll();
    void copy();
    void find();
    void filters();
    void options();
    void about();
    void aboutQt();

    // Change the view settings
    void toggleOverviewVisibility( bool isVisible );
    void toggleMainLineNumbersVisibility( bool isVisible );
    void toggleFilteredLineNumbersVisibility( bool isVisible );

    // Disable the follow mode checkbox and send the followSet signal down
    void disableFollow();

    // Update the line number displayed in the status bar.
    // Must be passed as the internal (starts at 0) line number.
    void lineNumberHandler( int line );

    // Instructs the widget to update the loading progress gauge
    void updateLoadingProgress( int progress );
    // Instructs the widget to display the 'normal' status bar,
    // without the progress gauge and with file info
    void displayNormalStatus( bool success );


  signals:
    // Is emitted when new settings must be used
    void optionsChanged();
    // Is emitted when the 'follow' option is enabled/disabled
    void followSet( bool checked );

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
    void setCurrentFile( const QString& fileName );
    void updateRecentFileActions();
    QString strippedName( const QString& fullFileName ) const;

    std::unique_ptr<Session> session_;
    SavedSearches *savedSearches;
    CrawlerWidget *crawlerWidget;
    RecentFiles& recentFiles;
    QString loadingFileName;
    QString currentFile;
    QString previousFile;

    enum { MaxRecentFiles = 5 };
    QAction *recentFileActions[MaxRecentFiles];
    MenuActionToolTipBehavior *recentFileActionBehaviors[MaxRecentFiles];
    QAction *separatorAction;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *toolsMenu;
    QMenu *helpMenu;

    InfoLine *infoLine;
    QLabel* lineNbField;
    QToolBar *toolBar;

    QAction *openAction;
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

    QIcon mainIcon_;

    // Multiplex signals to any of the CrawlerWidgets
    SignalMux signalMux_;
};

#endif

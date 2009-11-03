/*
 * Copyright (C) 2009 Nicolas Bonnefon and other contributors
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

#include <QMainWindow>
#include <QFileSystemWatcher>
#include "crawlerwidget.h"

class QAction;
class QLabel;

// Main window of the application, creates menus, toolbar and
// the CrawlerWidget
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow( QString fileName );

  protected:
    void closeEvent( QCloseEvent* event );
    // Drag and drop support
    void dragEnterEvent( QDragEnterEvent* event );
    void dropEvent( QDropEvent* event );

  private slots:
    void open();
    void openRecentFile();
    void copy();
    void reload();
    void filters();
    void options();
    void about();
    void aboutQt();

    // Instructs the window to signal the user the file has been updated
    void signalFileChanged( const QString& fileName );

  signals:
    // Is emitted when new settings must be used
    void optionsChanged();

  private:
    void createActions();
    void createMenus();
    void createContextMenu();
    void createToolBars();
    void createStatusBar();
    void createCrawler();
    void readSettings();
    void writeSettings();
    bool loadFile(const QString &fileName);
    void setCurrentFile( const QString &fileName,
            int fileSize, int fileNbLine );
    void updateRecentFileActions();
    QString strippedName(const QString &fullFileName);

    SavedSearches *savedSearches;
    CrawlerWidget *crawlerWidget;
    QStringList recentFiles;
    QString currentFile;
    QString previousFile;

    enum MonitoredFileStatus { UNCHANGED, DATA_ADDED, TRUNCATED };
    QFileSystemWatcher fileWatcher;
    MonitoredFileStatus fileChangedOnDisk_;
    qint64 fileSize_;

    enum { MaxRecentFiles = 5 };
    QAction *recentFileActions[MaxRecentFiles];
    QAction *separatorAction;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *toolsMenu;
    QMenu *helpMenu;

    QLabel *infoLine;
    QToolBar *toolBar;

    QAction *openAction;
    QAction *exitAction;
    QAction *copyAction;
    QAction *reloadAction;
    QAction *filtersAction;
    QAction *optionsAction;
    QAction *aboutAction;
    QAction *aboutQtAction;

    QIcon mainIcon_;
};

#endif

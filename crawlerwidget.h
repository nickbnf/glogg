#ifndef CRAWLERWIDGET_H
#define CRAWLERWIDGET_H

#include <QLabel>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "logmainview.h"
#include "filteredview.h"

#include "logdata.h"
#include "logfiltereddata.h"

class CrawlerWidget : public QSplitter
{
    Q_OBJECT

    public:
        CrawlerWidget(QWidget *parent=0);

        bool readFile(const QString &fileName);

    private slots:
        void searchClicked();
        void applyConfiguration();
        void updateFilteredView();
        void filteredViewSelection( int lineNb );

    private:
        LogMainView*    logMainView;
        QWidget*        bottomWindow;
        QLabel*         searchLabel;
        QLineEdit*      searchLineEdit;
        QPushButton*    searchButton;
        FilteredView*   filteredView;
        QLabel*         searchInfoLine;

        QVBoxLayout*    bottomMainLayout;
        QHBoxLayout*    searchLineLayout;

        static LogData  emptyLogData;
        static LogFilteredData  emptyLogFilteredData;

        LogData*        logData;
        LogFilteredData* logFilteredData;
};

#endif

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

class CrawlerWidget : public QSplitter
{
    Q_OBJECT

    public:
        CrawlerWidget(QWidget *parent=0);

        bool readFile(const QString &fileName);

    private slots:
        void searchClicked();
        void enableSearchButton(const QString& text);

    private:
        LogMainView*    logMainView;
        QWidget*        bottomWindow;
        QLabel*         searchLabel;
        QLineEdit*      searchLineEdit;
        QPushButton*    searchButton;
        FilteredView*   filteredView;

        QVBoxLayout*    bottomMainLayout;
        QHBoxLayout*    searchLineLayout;

        LogData*        logData;
        LogFilteredData* logFilteredData;
};

#endif

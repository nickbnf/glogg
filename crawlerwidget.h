#ifndef CRAWLERWIDGET_H
#define CRAWLERWIDGET_H

#include <QLabel>
#include <QSplitter>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "logmainview.h"
#include "filteredview.h"

class CrawlerWidget : public QSplitter
{
    Q_OBJECT

    private:
        LogMainView*    logMainView;
        QWidget*        bottomWindow;
        QLabel*         searchLabel;
        QLineEdit*      searchLineEdit;
        FilteredView*   filteredView;

        QVBoxLayout*    bottomMainLayout;
        QHBoxLayout*    searchLineLayout;

    public:
        CrawlerWidget(QWidget *parent=0);

        bool readFile(const QString &fileName);
};

#endif

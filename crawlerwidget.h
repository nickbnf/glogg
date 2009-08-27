#ifndef CRAWLERWIDGET_H
#define CRAWLERWIDGET_H

#include <QLabel>
#include <QSplitter>

#include "logmainview.h"
// #include "searchwindow.h"

class CrawlerWidget : public QSplitter
{
    Q_OBJECT

    private:
        LogMainView* logMainView;
  //      SearchWindow* searchWindow;
        QLabel* searchWindow;

    public:
        CrawlerWidget(QWidget *parent=0);

        bool readFile(const QString &fileName);
};

#endif

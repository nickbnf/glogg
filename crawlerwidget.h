#ifndef CRAWLERWIDGET_H
#define CRAWLERWIDGET_H

#include <QTextEdit>

class CrawlerWidget : public QTextEdit
{
    Q_OBJECT

    public:
        CrawlerWidget(QWidget *parent=0);

        bool readFile(const QString &fileName);
};

#endif

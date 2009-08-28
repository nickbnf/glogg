#include <Qt>

#include "crawlerwidget.h"

CrawlerWidget::CrawlerWidget(QWidget *parent) : QSplitter(parent)
{
    setOrientation(Qt::Vertical);

    logMainView = new LogMainView;
    searchWindow = new QLabel;
    addWidget(logMainView);
    addWidget(searchWindow);
}

bool CrawlerWidget::readFile(const QString &fileName)
{
    bool success = logMainView->readFile(fileName);
    if (success)
        searchWindow->setText(fileName);

    return success;
}
#if 0
CrawlerWidget::CrawlerWidget(QWidget *parent) : QTextEdit(parent)
{
    setLineWrapMode(QTextEdit::NoWrap);
    setReadOnly(true);
}

bool CrawlerWidget::readFile(const QString &fileName)
{
    QFile file(fileName);

    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        setPlainText(file.readAll());
        return true;
    }
    else
        return false;
}
#endif

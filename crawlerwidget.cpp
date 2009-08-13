#include <QFile>

#include "crawlerwidget.h"

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

#include <QFile>

#include "logmainview.h"

LogMainView::LogMainView(QWidget* parent) : QTextEdit(parent)
{
    setLineWrapMode(QTextEdit::NoWrap);
    setReadOnly(true);
}

bool LogMainView::readFile(const QString &fileName)
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

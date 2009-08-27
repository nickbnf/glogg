#ifndef LOGMAINVIEW_H
#define LOGMAINVIEW_H

#include <QTextEdit>

class LogMainView : public QTextEdit
{
    Q_OBJECT

    public:
        LogMainView(QWidget *parent=0);

        bool readFile(const QString &fileName);
};

#endif

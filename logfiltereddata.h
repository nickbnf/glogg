#ifndef LOGDATAVIEW_H
#define LOGDATAVIEW_H

#include <QByteArray>
#include <QList>
#include <QRegExp>
#include <QString>

class LogFilteredData {
    public:
        LogFilteredData(QByteArray* logData, QRegExp regExp);
        QString getLineString(int line);
        int getNbLine();

    private:
        QByteArray* data;
        QList<QString> lineList;
};

#endif


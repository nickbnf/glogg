#ifndef LOGFILTEREDDATA_H
#define LOGFILTEREDDATA_H

#include <QByteArray>
#include <QList>
#include <QRegExp>
#include <QString>

class LogFilteredData {
    public:
        LogFilteredData(QByteArray* logData, QRegExp regExp);
        QString getLineString(int line) const;
        int getNbLine() const;

    private:
        QList<QString> lineList;
};

#endif


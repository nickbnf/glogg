#ifndef LOGDATA_H
#define LOGDATA_H

#include <QByteArray>
#include <QString>

#include "logfiltereddata.h"

class LogData {
    public:
        LogData(const QByteArray &byteArray);
        QString getLineString(int line) const;
        int getNbLine() const;
        LogFilteredData* getNewFilteredData(QRegExp& regExp) const;

    private:
        QByteArray* data;
        int nbLines;
};

#endif

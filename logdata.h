#ifndef LOGDATA_H
#define LOGDATA_H

#include <QByteArray>
#include <QString>

class LogData {
    public:
        LogData(const QByteArray &byteArray);
        QString getLineString(int line);
        int getNbLine();

    private:
        QByteArray* data;
        int nbLines;
};

#endif

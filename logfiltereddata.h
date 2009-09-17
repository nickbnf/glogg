#ifndef LOGFILTEREDDATA_H
#define LOGFILTEREDDATA_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QRegExp>

#include "abstractlogdata.h"

class LogFilteredData : public AbstractLogData {
    Q_OBJECT

    public:
        /// @brief Create an empty LogData
        LogFilteredData();
        LogFilteredData(QByteArray* logData, QRegExp regExp);
        LogFilteredData(QStringList* logData, QRegExp regExp);

        void runSearch();

    signals:
        void newDataAvailable();

    private:
        QString doGetLineString(int line) const;
        int doGetNbLine() const;

        QList<QString> lineList;

        QStringList* sourceLogData;
        QRegExp currentRegExp;
        bool searchDone;
};

#endif


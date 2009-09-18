#ifndef LOGFILTEREDDATA_H
#define LOGFILTEREDDATA_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QRegExp>

#include "abstractlogdata.h"

/// @brief Class encapsulating a matching line
class matchingLine {
    public:
        matchingLine( int line, QString str ) { lineNumber = line; lineString = str; };

        int lineNumber;
        QString lineString;
};

class LogFilteredData : public AbstractLogData {
    Q_OBJECT

    public:
        /// @brief Create an empty LogData
        LogFilteredData();
        LogFilteredData(QByteArray* logData, QRegExp regExp);
        LogFilteredData(QStringList* logData, QRegExp regExp);

        void runSearch();
        int getMatchingLineNumber(int lineNum) const;

    signals:
        void newDataAvailable();

    private:
        QString doGetLineString(int line) const;
        int doGetNbLine() const;

        QList<matchingLine> matchingLineList;

        QStringList* sourceLogData;
        QRegExp currentRegExp;
        bool searchDone;
};

#endif


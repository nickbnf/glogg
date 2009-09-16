#include "log.h"

#include <QString>

#include "logfiltereddata.h"

// Deprecated following QListString implementation
LogFilteredData::LogFilteredData(QByteArray* logData, QRegExp regExp) : AbstractLogData()
{
    LOG(logDEBUG) << "Entering LogFilteredData constructor";

    int pos = 0, end = 0;
    while ( (end = logData->indexOf("\n", pos)) != -1 ) {
        const QString string = QString(logData->mid(pos, end-pos));
        pos = end+1;
        if ( regExp.indexIn(string) != -1 )
            lineList.append(string);
    }

    LOG(logDEBUG) << "End LogFilteredData";
}

LogFilteredData::LogFilteredData(QStringList* logData, QRegExp regExp) : AbstractLogData()
{
    LOG(logDEBUG) << "Entering LogFilteredData constructor";

    QStringList::iterator i;
    for ( i = logData->begin(); i != logData->end(); ++i ) {
        if ( regExp.indexIn( *i ) != -1 ) {
            const int length = i->length();
            if ( length > maxLength )
                maxLength = length;
            lineList.append( *i );
        }
    }

    LOG(logDEBUG) << "End LogFilteredData";
}

QString LogFilteredData::doGetLineString(int line) const
{
    QString string = lineList[line];

    return string;
}

int LogFilteredData::doGetNbLine() const
{
    return lineList.length();
}

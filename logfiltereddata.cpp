#include "log.h"

#include <QString>

#include "logfiltereddata.h"

LogFilteredData::LogFilteredData() : AbstractLogData()
{
    matchingLineList = QList<matchingLine>();
}

LogFilteredData::LogFilteredData(QStringList* logData, QRegExp regExp) : AbstractLogData()
{
    sourceLogData = logData;
    currentRegExp = regExp;

    searchDone = false;
}

void LogFilteredData::runSearch()
{
    LOG(logDEBUG) << "Entering runSearch";

    int lineNum = 0;
    for ( QStringList::iterator i = sourceLogData->begin();
          i != sourceLogData->end();
          ++i, ++lineNum )
    {
        if ( currentRegExp.indexIn( *i ) != -1 ) {
            const int length = i->length();
            if ( length > maxLength )
                maxLength = length;
            matchingLine match( lineNum, *i );
            matchingLineList.append( match );
        }
    }

    searchDone = true;
    emit newDataAvailable();

    LOG(logDEBUG) << "End runSearch";
}

int LogFilteredData::getMatchingLineNumber(int lineNum) const
{
    int matchingNb = matchingLineList[lineNum].lineNumber;

    return matchingNb;
}

QString LogFilteredData::doGetLineString(int lineNum) const
{
    QString string = matchingLineList[lineNum].lineString;

    return string;
}

int LogFilteredData::doGetNbLine() const
{
    return matchingLineList.size();
}

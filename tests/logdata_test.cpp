#include "logdata_test.h"
#include "logdata.h"

#if QT_VERSION < 0x040500
#define QBENCHMARK
#endif

static const int NB_LINES = 99999;
static const int LINE_PER_PAGE = 70;

#if 0
void TestLogData::simpleLoad()
{
    QByteArray array = generateData();

    LogData* logData;

    QBENCHMARK {
        logData = new LogData(array);
    }
    QVERIFY(logData != NULL);
    QVERIFY(logData->getNbLine() == NB_LINES);

    delete logData;
}

void TestLogData::sequentialRead()
{
    LogData* logData;

    {
        QByteArray array = generateData();
        logData = new LogData(array);
    }

    // Read all lines sequentially
    QString s;
    QBENCHMARK {
        for (int i = 0; i < NB_LINES; i++)
            s = logData->getLineString(i);
    }
    QVERIFY(s.length() > 0);

    delete logData;
}
#endif

void TestLogData::randomPageRead()
{
    LogData* logData;

    {
        QByteArray array = generateData();
        logData = new LogData(array);
    }

    // Read page by page from the beginning and the end
    QString s;
    QBENCHMARK {
        for (int page = 0; page < (NB_LINES/LINE_PER_PAGE)-1; page++)
        {
            for (int i = page*LINE_PER_PAGE; i < ((page+1)*LINE_PER_PAGE); i++)
                s = logData->getLineString(i);
            int page_from_end = (NB_LINES/LINE_PER_PAGE) - page - 1;
            for (int i = page_from_end*LINE_PER_PAGE; i < ((page_from_end+1)*LINE_PER_PAGE); i++)
                s = logData->getLineString(i);
        }
    }
    QVERIFY(s.length() > 0);

    delete logData;
}

QByteArray TestLogData::generateData()
{
    const char* line="LOGDATA is a part of LogCrawler, we are going to test it thoroughly, this is line %5d\n";
    char newLine[90];

    QByteArray array = QByteArray();

    for (int i = 0; i < NB_LINES; i++) {
        sprintf(newLine, line, i);
        array += line;
    }

    return array;
}

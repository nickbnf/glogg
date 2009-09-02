#include "logdata_test.h"
#include "logdata.h"

#if QT_VERSION < 0x040500
#define QBENCHMARK
#endif

static const int NB_LINES = 9999;

void TestLogData::simpleLoad()
{
    char* line="LOGDATA is a part of LogCrawler, we are going to test it thoroughly, this is line 000000\n";

    LogData* logData;
    QByteArray array = QByteArray(line);

    for (int i = 1; i < NB_LINES; i++)
        array += line;

    QBENCHMARK {
        logData = new LogData(array);
    }
    QVERIFY(logData != NULL);
    QVERIFY(logData->getNbLine() == NB_LINES);

    // Read all lines sequentially
    QString s;
    QBENCHMARK {
        for (int i = 0; i < NB_LINES; i++)
            s = logData->getLineString(i);
    }
    QVERIFY(s == QString(line));

    delete logData;
}

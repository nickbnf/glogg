#include "logdata_test.h"
#include "logdata.h"

static const int NB_LINES = 999999;

void TestLogData::simpleLoad()
{
    char* line="LOGDATA is a part of LogCrawler, we are going to test it thoroughly, this is line 000000\n";

    QByteArray array = QByteArray(line);

    for (int i = 1; i < NB_LINES; i++)
        array += line;

    LogData* logData = new LogData(array);
    QVERIFY(logData != NULL);
    QVERIFY(logData->getNbLine() == NB_LINES);

    // Read all lines sequentially
    QString s;
    for (int i = 1; i < NB_LINES; i++)
        s = logData->getLineString(i);
    QVERIFY(s == line);


    delete logData;
}

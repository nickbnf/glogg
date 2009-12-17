#include <QTest>

#include "testlogdata.h"
#include "testlogfiltereddata.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    int retval(0);
//    retval += QTest::qExec(&TestLogData(), argc, argv);
    retval += QTest::qExec(&TestLogFilteredData(), argc, argv);

    return (retval ? 1 : 0);

}

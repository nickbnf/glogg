#include <QApplication>
#include <QTest>

#include "logdata_test.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    int retval(0);
    retval +=QTest::qExec(&TestLogData(), argc, argv);

    return (retval ? 1 : 0);

}

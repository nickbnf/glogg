#include <QByteArray>
#include <QtTest/QtTest>

class TestLogData: public QObject
{
    Q_OBJECT
    private slots:
        // void simpleLoad();
        // void sequentialRead();
        void randomPageRead();

    private:
        QByteArray generateData();
};

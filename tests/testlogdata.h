#include <QMutex>
#include <QWaitCondition>
#include <QtTest/QtTest>

class TestLogData: public QObject
{
    Q_OBJECT

    private slots:
        void initTestCase();

        void simpleLoad();
        void multipleLoad();
        void changingFile();
        void sequentialRead();
        void randomPageRead();

    public slots:
        void loadingFinished();

    private:
        bool generateDataFiles();
};

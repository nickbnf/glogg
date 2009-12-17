#include <QMutex>
#include <QWaitCondition>
#include <QtTest/QtTest>

class TestLogFilteredData: public QObject
{
    Q_OBJECT

    private slots:
        void initTestCase();

        void simpleSearch();
        void multipleSearch();

    public slots:
        void loadingFinished();
        void searchProgressed( int completion, int nbMatches );

    private:
        bool generateDataFiles();
};

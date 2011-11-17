#include <QMutex>
#include <QWaitCondition>
#include <QtTest/QtTest>

class LogData;
class LogFilteredData;

class TestLogFilteredData: public QObject
{
    Q_OBJECT

    public:
        TestLogFilteredData();

    private slots:
        void initTestCase();

        void simpleSearch();
        void multipleSearch();
        void updateSearch();

    public slots:
        void loadingFinished();
        void searchProgressed( int completion, int nbMatches );

    private:
        bool generateDataFiles();

        void simpleSearchTest();
        void multipleSearchTest();
        void updateSearchTest();

        std::pair<int,int> waitSearchProgressed();
        void waitLoadingFinished();
        void signalSearchProgressedRead();
        void signalLoadingFinishedRead();

        LogData* logData_;
        LogFilteredData* filteredData_;

        // Synchronisation variables (protected by the two mutexes)
        bool loadingFinished_received_;
        bool loadingFinished_read_;
        bool searchProgressed_received_;
        bool searchProgressed_read_;

        int searchLastMatches_;
        int searchLastProgress_;

        QMutex loadingFinishedMutex_;
        QMutex searchProgressedMutex_;

        QWaitCondition loadingFinishedCondition_;
        QWaitCondition searchProgressedCondition_;
};

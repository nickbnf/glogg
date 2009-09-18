#include "log.h"

#include <Qt>
#include <QFile>

#include "crawlerwidget.h"

#include "configuration.h"

LogData         CrawlerWidget::emptyLogData;
LogFilteredData CrawlerWidget::emptyLogFilteredData;

CrawlerWidget::CrawlerWidget(QWidget *parent) : QSplitter(parent)
{
    setOrientation(Qt::Vertical);

    // Initialize internal data
    logData         = &emptyLogData;
    logFilteredData = &emptyLogFilteredData;

    logMainView =  new LogMainView( logData );
    bottomWindow = new QWidget;
    filteredView = new FilteredView( logFilteredData );

    // Construct the Search Info line
    searchInfoLine = new QLabel();
    searchInfoLine->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    searchInfoLine->setLineWidth( 1 );

    // Construct the Search line
    searchLabel =  new QLabel(tr("&Text: "));
    searchLineEdit = new QLineEdit;
    searchLabel->setBuddy(searchLineEdit);
    searchButton = new QPushButton(tr("&Search"));
    searchButton->setEnabled(false);

    QHBoxLayout* searchLineLayout = new QHBoxLayout;
    searchLineLayout->addWidget(searchLabel);
    searchLineLayout->addWidget(searchLineEdit);
    searchLineLayout->addWidget(searchButton);
    searchLineLayout->setContentsMargins(6, 0, 6, 0);

    // Construct the bottom window
    QVBoxLayout* bottomMainLayout = new QVBoxLayout;
    bottomMainLayout->addLayout(searchLineLayout);
    bottomMainLayout->addWidget(searchInfoLine);
    bottomMainLayout->addWidget(filteredView);
    bottomMainLayout->setContentsMargins(2, 1, 2, 1);
    bottomWindow->setLayout(bottomMainLayout);

    addWidget(logMainView);
    addWidget(bottomWindow);

    // Connect the signals
    connect(searchLineEdit, SIGNAL(returnPressed()),
            searchButton, SIGNAL(clicked()));
    connect(searchButton, SIGNAL( clicked() ),
            this, SLOT( searchClicked() ) );
    connect(logMainView, SIGNAL( newSelection( int ) ),
            logMainView, SLOT( update() ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            this, SLOT( filteredViewSelection( int ) ) );
    connect(filteredView, SIGNAL( newSelection( int ) ),
            filteredView, SLOT( update() ) );
}

/*
 * Slots
 */
void CrawlerWidget::searchClicked()
{
    LOG(logDEBUG) << "searchClicked received";

    // Delete (and disconnect the object)
    if ( logFilteredData != &emptyLogFilteredData )
        delete logFilteredData;

    QString text = searchLineEdit->text();
    QRegExp regexp(text);

    // Create the new LogFilteredData
    if ( !text.isEmpty() ) {
        logFilteredData = logData->getNewFilteredData(regexp);
        connect( logFilteredData, SIGNAL( newDataAvailable() ),
                this, SLOT( updateFilteredView() ) );
        logFilteredData->runSearch();
    }
    else {
        logFilteredData = &emptyLogFilteredData;
        // We won't receive an event from the emptyLogFilteredData
        searchInfoLine->setText( "" );
        filteredView->updateData( logFilteredData );
    }
}

void CrawlerWidget::updateFilteredView()
{
    LOG(logDEBUG) << "updateFilteredView received.";

    searchInfoLine->setText( tr("Found %1 match%2.")
            .arg( logFilteredData->getNbLine() )
            .arg( logFilteredData->getNbLine() > 1 ? "es" : "" ) );
    filteredView->updateData(logFilteredData);
}

/// @brief Slot called when the user select a line in the filtered view
void CrawlerWidget::filteredViewSelection( int lineNb )
{
    int mainViewLine = logFilteredData->getMatchingLineNumber( lineNb );
    logMainView->displayLine( mainViewLine );  // FIXME: should be done with a signal.
}

void CrawlerWidget::applyConfiguration()
{
    QFont font = Config().mainFont();

    LOG(logDEBUG) << "CrawlerWidget::applyConfiguration";

    logMainView->setFont(font);
    filteredView->setFont(font);

    logMainView->update();
    filteredView->update();
}

bool CrawlerWidget::readFile(const QString &fileName)
{
    QFile file(fileName);

    if (file.open(QFile::ReadOnly | QFile::Text)) {
        LogData* oldLogData = logData;
        logData = new LogData(file.readAll());

        logMainView->updateData(logData);

        if (oldLogData != &emptyLogData)
            delete oldLogData;

        return true;
    }
    else {
        return false;
    }
}

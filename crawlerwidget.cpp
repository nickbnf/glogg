#include "log.h"

#include <Qt>
#include <QFile>

#include "crawlerwidget.h"

CrawlerWidget::CrawlerWidget(QWidget *parent) : QSplitter(parent)
{
    setOrientation(Qt::Vertical);

    logMainView =  new LogMainView;
    bottomWindow = new QWidget;
    filteredView = new FilteredView;

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
    searchLineLayout->setContentsMargins(8, 0, 8, 0);

    // Construct the bottom window
    QVBoxLayout* bottomMainLayout = new QVBoxLayout;
    bottomMainLayout->addLayout(searchLineLayout);
    bottomMainLayout->addWidget(filteredView);
    bottomMainLayout->setContentsMargins(0, 0, 0, 0);
    bottomWindow->setLayout(bottomMainLayout);

    addWidget(logMainView);
    addWidget(bottomWindow);

    // Initialize internal data
    logData = NULL;

    // Connect the signals
    connect(searchLineEdit, SIGNAL(textChanged(const QString&)),
            this, SLOT(enableSearchButton(const QString&)));
    connect(searchButton, SIGNAL(clicked()),
            this, SLOT(searchClicked()));
}

void CrawlerWidget::searchClicked()
{
    LOG(logDEBUG) << "searchClicked received";

    QString text = searchLineEdit->text();
    QRegExp regexp(text);
    logFilteredData = logData->getNewFilteredData(regexp);
    filteredView->updateData(logFilteredData);
}

void CrawlerWidget::enableSearchButton(const QString& text)
{
    searchButton->setEnabled(!text.isEmpty());
}

bool CrawlerWidget::readFile(const QString &fileName)
{
    QFile file(fileName);

    if (file.open(QFile::ReadOnly | QFile::Text)) {
        LogData* oldLogData = logData;
        logData = new LogData(file.readAll());

        logMainView->updateData(logData);

        if (oldLogData)
            delete oldLogData;

        return true;
    }
    else {
        return false;
    }
}

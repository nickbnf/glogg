#include <Qt>

#include "crawlerwidget.h"

CrawlerWidget::CrawlerWidget(QWidget *parent) : QSplitter(parent)
{
    setOrientation(Qt::Vertical);

    logMainView =  new LogMainView;
    bottomWindow = new QWidget;
    filteredView = new FilteredView;

    // Construct the Search line
    searchLabel =  new QLabel(tr("&Search: "));
    searchLineEdit = new QLineEdit;
    searchLabel->setBuddy(searchLineEdit);

    QHBoxLayout* searchLineLayout = new QHBoxLayout;
    searchLineLayout->addWidget(searchLabel);
    searchLineLayout->addWidget(searchLineEdit);
    searchLineLayout->setContentsMargins(8, 0, 8, 0);

    // Construct the bottom window
    QVBoxLayout* bottomMainLayout = new QVBoxLayout;
    bottomMainLayout->addLayout(searchLineLayout);
    bottomMainLayout->addWidget(filteredView);
    bottomMainLayout->setContentsMargins(0, 0, 0, 0);
    bottomWindow->setLayout(bottomMainLayout);

    addWidget(logMainView);
    addWidget(bottomWindow);
}

bool CrawlerWidget::readFile(const QString &fileName)
{
    bool success = logMainView->readFile(fileName);
    /*
    if (success)
        searchWindow->setText(fileName);
        */

    return success;
}

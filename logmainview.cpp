#include <iostream>

#include <QFile>
#include <QRect>
#include <QPaintEvent>
#include <QPainter>
#include <QFontMetrics>

#include "log.h"

#include "common.h"
#include "logmainview.h"

LogMainView::LogMainView(QWidget* parent) : QWidget(parent)
{
    logData = NULL;
}

void LogMainView::paintEvent(QPaintEvent* paintEvent)
{
    QRect invalidRect = paintEvent->rect();
    if ( (invalidRect.isEmpty()) || (logData == NULL) )
        return;

    int lastLine = min2( logData->getNbLine(), firstLine + getNbVisibleLines() );

    {
        QPainter painter(this);
        int fontHeight = painter.fontMetrics().height();
        painter.fillRect(invalidRect, Qt::white);
        painter.setPen( Qt::black );
        for (int i = firstLine; i < lastLine; i++) {
            int yPos = (i-firstLine + 1) * fontHeight;
            painter.drawText(0, yPos, logData->getLineString(i));
        }
    }

    std::cout << "paintEvent!" << std::endl;
}

int LogMainView::getNbVisibleLines()
{
    QFontMetrics fm = fontMetrics();
    return height()/fm.height() + 1;
}

bool LogMainView::readFile(const QString &fileName)
{
    QFile file(fileName);

    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        if (logData)
            delete logData;
        logData = new LogData(file.readAll());

        firstLine = 0;
        update();
        return true;
    }
    else
        return false;
}

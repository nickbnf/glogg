#ifndef QACTIONEXTENDED_H
#define QACTIONEXTENDED_H

#include <QAction>
#include <iostream>
#include <functional>

using namespace std;
class QActionExtended: public QAction
{
    Q_OBJECT

public:

    QActionExtended(string pluginName, function<void(string)> action, QObject *parent = nullptr):QAction(tr(""), parent),action_(action),pluginName_(pluginName)
    {
    }
private slots:
    void showPluginUI()
    {
        cout << __FUNCTION__ << "\n";
        action_(pluginName_);
    }
private:
    function<void(string)> action_;
    string pluginName_;
};

#endif // QACTIONEXTENDED_H
#ifndef PLUGINSDIALOG_H
#define PLUGINSDIALOG_H

#include <QDialog>

namespace Ui {
class PluginsDialog;
}

class PluginsDialog : public QDialog
{
    Q_OBJECT

public:
signals:
    void pluginsOptionsChanged();
public:
    explicit PluginsDialog(QWidget *parent = nullptr);
    ~PluginsDialog();

private:
    Ui::PluginsDialog *ui;
};

#endif // PLUGINSDIALOG_H

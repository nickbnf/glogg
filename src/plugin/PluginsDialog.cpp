#include "PythonPlugin.h"

#include "PluginsDialog.h"
#include "ui_PluginsDialog.h"

PluginsDialog::PluginsDialog(PythonPlugin* pyPlugin, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PluginsDialog),
    pluginSet_(Persistent<PluginSet>( "pluginSet" )),
    pythonPlugin_(pyPlugin)
{
    ui->setupUi(this);

    ui->pluginSystemEnabled->setChecked(pluginSet_->isPluginSystemEnabled());
    createPluginClassList();
}

PluginsDialog::~PluginsDialog()
{
    delete ui;
}

void PluginsDialog::createPluginClassList()
{
    for(const auto& type: pythonPlugin_->getTypes()){
        QListWidgetItem* item = new QListWidgetItem(type.name.c_str(), ui->listWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
        item->setCheckState(Qt::Unchecked); // AND initialize check state
    }
}

void PluginsDialog::on_buttonBox_accepted()
{
    GetPersistentInfo().save("pluginSet");
}

void PluginsDialog::on_pluginSystemEnabled_clicked(bool checked)
{
    pluginSet_->setPluginSystemEnabled(checked);
}

void PluginsDialog::on_listWidget_itemClicked(QListWidgetItem *item)
{
    bool checked = item->checkState();
    QString text = item->text();
    pythonPlugin_->setPluginState(text.toStdString(), checked);
}

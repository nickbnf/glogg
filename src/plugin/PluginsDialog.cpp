#include "PluginsDialog.h"
#include "ui_PluginsDialog.h"

PluginsDialog::PluginsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PluginsDialog)
{
    ui->setupUi(this);

    QListWidgetItem* item = new QListWidgetItem("item", ui->listWidget);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
    item->setCheckState(Qt::Unchecked); // AND initialize check state
}

PluginsDialog::~PluginsDialog()
{
    delete ui;
}

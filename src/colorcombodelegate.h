#ifndef COLORCOMBODELEGATE_H
#define COLORCOMBODELEGATE_H

#include <QStyledItemDelegate>

class ColorComboDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:

    ColorComboDelegate(QObject* parent=0);
    virtual ~ColorComboDelegate();
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const;
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
};

#endif // COLORCOMBODELEGATE_H


#include "colorcombodelegate.h"
#include <QComboBox>

ColorComboDelegate::ColorComboDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

ColorComboDelegate::~ColorComboDelegate()
{
}

QWidget* ColorComboDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // Create the combobox and populate it
    QComboBox* cb = new QComboBox(parent);
    const QStringList colorNames = QStringList()
        // Basic 16 HTML colors (minus greys):
        << "black"
        << "white"
        << "maroon"
        << "red"
        << "purple"
        << "fuchsia"
        << "green"
        << "lime"
        << "olive"
        << "yellow"
        << "navy"
        << "blue"
        << "teal"
        << "aqua"
        // Greys
        << "gainsboro"
        << "lightgrey"
        << "silver"
        << "darkgrey"
        << "grey"
        << "dimgrey"
        // Reds
        << "tomato"
        << "orangered"
        << "orange"
        << "crimson"
        << "darkred"
        // Greens
        << "greenyellow"
        << "lightgreen"
        << "darkgreen"
        << "lightseagreen"
        // Blues
        << "lightcyan"
        << "darkturquoise"
        << "steelblue"
        << "lightblue"
        << "royalblue"
        << "darkblue"
        << "midnightblue"
        // Browns
        << "bisque"
        << "tan"
        << "sandybrown"
        << "chocolate";

    for ( QStringList::const_iterator i = colorNames.constBegin();
            i != colorNames.constEnd(); ++i ) {
        QPixmap solidPixmap( 20, 10 );
        solidPixmap.fill( QColor( *i ) );
        QIcon solidIcon { solidPixmap };

        cb->addItem( solidIcon, *i );
    }

    return cb;
}

void ColorComboDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (QComboBox* cb = qobject_cast<QComboBox*>(editor)) {
       // get the index of the text in the combobox that matches the current value of the itenm
       QString currentText = index.data(Qt::EditRole).toString();
       int cbIndex = cb->findText(currentText);
       // if it is valid, adjust the combobox
       if (cbIndex >= 0)
           cb->setCurrentIndex(cbIndex);
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void ColorComboDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
        // save the current text of the combo box as the current value of the item
        model->setData(index, cb->currentText(), Qt::EditRole);
    else
        QStyledItemDelegate::setModelData(editor, model, index);
}

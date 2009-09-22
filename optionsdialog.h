#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QComboBox>

#include "configuration.h"

class OptionsDialog : public QDialog
{
    Q_OBJECT

    public:
        OptionsDialog(QWidget* parent = 0);

    signals:
        /// Is emitted when new settings must be used
        void optionsChanged();

    private slots:
        /*
        void applyClicked();
        */
        void updateFontSize(const QString& text);
        void updateConfigFromDialog();

    private:
        QPushButton* okButton;
        QPushButton* cancelButton;
        QPushButton* applyButton;

        QComboBox*   fontFamilyBox;
        QComboBox*   fontSizeBox;

        void setupFontList();
        void updateDialogFromConfig();
};

#endif

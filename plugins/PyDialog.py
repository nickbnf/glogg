#!/usr/bin/env python3
# coding: utf-8

#import sys
## -*- coding: utf-8 -*-

## Form implementation generated from reading ui file 'PyDialog.ui'
##
## Created by: PyQt5 UI code generator 5.14.0
##
## WARNING! All changes made in this file will be lost!

from PyDialog_ui import Ui_Dialog
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QApplication, QMainWindow

import handlers


class MyForm(QtWidgets.QDialog):
    def __init__(self, plugin, parent=None):
        QtWidgets.QDialog.__init__(self, parent)
        self.ui = Ui_Dialog()
        self.ui.setupUi(self)
        self.plugin = plugin

        #QtCore.QObject.connect(self.ui.buttonBox, QtCore.SIGNAL("clicked()"), self.calculate_value)
        self.ui.pushButton.clicked.connect(self.funkcja)
        self.ui.pushButtonColumns.clicked.connect(self.funkcja)

    def funkcja(self):
        print("funkcja")
        #self.ui.lineEdit.setText("dupa")
        self.plugin.update_app_views()

    def getValue(self):
        return self.ui.lineEdit.text()

    def getColumns(self):
        return self.ui.lineEditColumns.text()



#app = QtGui.QApplication()
#app = QApplication([])

#sys.exit(app.exec_())

# myapp = MyForm()
# myapp.show()
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


#class Ui_Dialog(object):
#    def setupUi(self, Dialog):
#        Dialog.setObjectName("Dialog")
#        Dialog.resize(400, 300)
#        self.buttonBox = QtWidgets.QDialogButtonBox(Dialog)
#        self.buttonBox.setGeometry(QtCore.QRect(30, 240, 341, 32))
#        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
#        self.buttonBox.setStandardButtons(QtWidgets.QDialogButtonBox.Cancel|QtWidgets.QDialogButtonBox.Ok)
#        self.buttonBox.setObjectName("buttonBox")
#        self.lineEdit = QtWidgets.QLineEdit(Dialog)
#        self.lineEdit.setGeometry(QtCore.QRect(20, 20, 241, 25))
#        self.lineEdit.setObjectName("lineEdit")
#        self.pushButton = QtWidgets.QPushButton(Dialog)
#        self.pushButton.setGeometry(QtCore.QRect(280, 20, 89, 25))
#        self.pushButton.setObjectName("pushButton")

#        self.retranslateUi(Dialog)
#        self.buttonBox.accepted.connect(Dialog.accept)
#        self.buttonBox.rejected.connect(Dialog.reject)
#        QtCore.QMetaObject.connectSlotsByName(Dialog)

#    def retranslateUi(self, Dialog):
#        _translate = QtCore.QCoreApplication.translate
#        Dialog.setWindowTitle(_translate("Dialog", "Dialog"))
#        self.pushButton.setText(_translate("Dialog", "PushButton"))


class MyForm(QtWidgets.QDialog):
    def __init__(self, parent=None):
        QtWidgets.QDialog.__init__(self, parent)
        self.ui = Ui_Dialog()
        self.ui.setupUi(self)

        #QtCore.QObject.connect(self.ui.buttonBox, QtCore.SIGNAL("clicked()"), self.calculate_value)
        self.ui.pushButton.clicked.connect(self.funkcja)

    def funkcja(self):
        print("funkcja")
        self.ui.lineEdit.setText("dupa")

    def getValue(self):
        return self.ui.lineEdit.text()

    def getColumns(self):
        return self.ui.lineEditColumns.text()



#app = QtGui.QApplication()
#app = QApplication([])

#sys.exit(app.exec_())

# myapp = MyForm()
# myapp.show()
# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file '/home/roman.szul/projects/glogg/glogg/src/PyDialog.ui'
#
# Created by: PyQt5 UI code generator 5.14.0
#
# WARNING! All changes made in this file will be lost!


from PyQt5 import QtCore, QtGui, QtWidgets


class Ui_Dialog(object):
    def setupUi(self, Dialog):
        Dialog.setObjectName("Dialog")
        Dialog.resize(770, 309)
        self.buttonBox = QtWidgets.QDialogButtonBox(Dialog)
        self.buttonBox.setGeometry(QtCore.QRect(30, 240, 341, 32))
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtWidgets.QDialogButtonBox.Cancel|QtWidgets.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.lineEdit = QtWidgets.QLineEdit(Dialog)
        self.lineEdit.setGeometry(QtCore.QRect(20, 20, 241, 25))
        self.lineEdit.setObjectName("lineEdit")
        self.pushButton = QtWidgets.QPushButton(Dialog)
        self.pushButton.setGeometry(QtCore.QRect(280, 20, 89, 25))
        self.pushButton.setObjectName("pushButton")
        self.pushButtonColumns = QtWidgets.QPushButton(Dialog)
        self.pushButtonColumns.setGeometry(QtCore.QRect(280, 90, 89, 25))
        self.pushButtonColumns.setObjectName("pushButtonColumns")
        self.lineEditColumns = QtWidgets.QLineEdit(Dialog)
        self.lineEditColumns.setGeometry(QtCore.QRect(20, 90, 241, 25))
        self.lineEditColumns.setObjectName("lineEditColumns")
        self.pushButtonRegex = QtWidgets.QPushButton(Dialog)
        self.pushButtonRegex.setGeometry(QtCore.QRect(650, 160, 89, 25))
        self.pushButtonRegex.setObjectName("pushButtonRegex")
        self.lineEditRegex = QtWidgets.QLineEdit(Dialog)
        self.lineEditRegex.setGeometry(QtCore.QRect(20, 130, 721, 25))
        self.lineEditRegex.setObjectName("lineEditRegex")
        self.frame = QtWidgets.QFrame(Dialog)
        self.frame.setGeometry(QtCore.QRect(430, 180, 171, 80))
        self.frame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        self.frame.setFrameShadow(QtWidgets.QFrame.Raised)
        self.frame.setObjectName("frame")
        self.radioButtonColumns = QtWidgets.QRadioButton(self.frame)
        self.radioButtonColumns.setGeometry(QtCore.QRect(10, 10, 112, 23))
        self.radioButtonColumns.setChecked(True)
        self.radioButtonColumns.setObjectName("radioButtonColumns")
        self.radioButtonRegex = QtWidgets.QRadioButton(self.frame)
        self.radioButtonRegex.setGeometry(QtCore.QRect(10, 40, 112, 23))
        self.radioButtonRegex.setObjectName("radioButtonRegex")

        self.retranslateUi(Dialog)
        self.buttonBox.accepted.connect(Dialog.accept)
        self.buttonBox.rejected.connect(Dialog.reject)
        QtCore.QMetaObject.connectSlotsByName(Dialog)

    def retranslateUi(self, Dialog):
        _translate = QtCore.QCoreApplication.translate
        Dialog.setWindowTitle(_translate("Dialog", "Dialog"))
        self.pushButton.setText(_translate("Dialog", "PushButton"))
        self.pushButtonColumns.setText(_translate("Dialog", "Columns"))
        self.pushButtonRegex.setText(_translate("Dialog", "update"))
        self.radioButtonColumns.setText(_translate("Dialog", "Columns"))
        self.radioButtonRegex.setText(_translate("Dialog", "Regex"))

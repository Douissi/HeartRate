/********************************************************************************
** Form generated from reading UI file 'qcvwidget.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QCVWIDGET_H
#define UI_QCVWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QcvWidget
{
public:
    QLabel *labelView;
    QPushButton *pushButtonPlay;

    void setupUi(QWidget *QcvWidget)
    {
        if (QcvWidget->objectName().isEmpty())
            QcvWidget->setObjectName(QStringLiteral("QcvWidget"));
        QcvWidget->resize(660, 438);
        labelView = new QLabel(QcvWidget);
        labelView->setObjectName(QStringLiteral("labelView"));
        labelView->setGeometry(QRect(40, 10, 561, 371));
        labelView->setStyleSheet(QStringLiteral("border: 1px solid;"));
        pushButtonPlay = new QPushButton(QcvWidget);
        pushButtonPlay->setObjectName(QStringLiteral("pushButtonPlay"));
        pushButtonPlay->setGeometry(QRect(280, 390, 80, 23));

        retranslateUi(QcvWidget);

        QMetaObject::connectSlotsByName(QcvWidget);
    } // setupUi

    void retranslateUi(QWidget *QcvWidget)
    {
        QcvWidget->setWindowTitle(QApplication::translate("QcvWidget", "Form", 0));
        labelView->setText(QString());
        pushButtonPlay->setText(QApplication::translate("QcvWidget", "start", 0));
    } // retranslateUi

};

namespace Ui {
    class QcvWidget: public Ui_QcvWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QCVWIDGET_H

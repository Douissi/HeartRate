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
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QcvWidget
{
public:
    QLabel *labelView;
    QTextBrowser *textBrowser;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QLCDNumber *lcdNumber;
    QPushButton *pushButtonPlay;

    void setupUi(QWidget *QcvWidget)
    {
        if (QcvWidget->objectName().isEmpty())
            QcvWidget->setObjectName(QStringLiteral("QcvWidget"));
        QcvWidget->resize(758, 633);
        labelView = new QLabel(QcvWidget);
        labelView->setObjectName(QStringLiteral("labelView"));
        labelView->setGeometry(QRect(40, 10, 451, 371));
        labelView->setStyleSheet(QStringLiteral("border: 1px solid;"));
        textBrowser = new QTextBrowser(QcvWidget);
        textBrowser->setObjectName(QStringLiteral("textBrowser"));
        textBrowser->setGeometry(QRect(580, 30, 171, 31));
        widget = new QWidget(QcvWidget);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setGeometry(QRect(580, 70, 171, 201));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        lcdNumber = new QLCDNumber(widget);
        lcdNumber->setObjectName(QStringLiteral("lcdNumber"));

        verticalLayout->addWidget(lcdNumber);

        pushButtonPlay = new QPushButton(widget);
        pushButtonPlay->setObjectName(QStringLiteral("pushButtonPlay"));

        verticalLayout->addWidget(pushButtonPlay);


        retranslateUi(QcvWidget);

        QMetaObject::connectSlotsByName(QcvWidget);
    } // setupUi

    void retranslateUi(QWidget *QcvWidget)
    {
        QcvWidget->setWindowTitle(QApplication::translate("QcvWidget", "Form", 0));
        labelView->setText(QString());
        textBrowser->setHtml(QApplication::translate("QcvWidget", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Heart Rate Estimation</span></p></body></html>", 0));
        pushButtonPlay->setText(QApplication::translate("QcvWidget", "start", 0));
    } // retranslateUi

};

namespace Ui {
    class QcvWidget: public Ui_QcvWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QCVWIDGET_H

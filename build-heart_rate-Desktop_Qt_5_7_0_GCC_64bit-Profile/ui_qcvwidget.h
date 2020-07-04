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
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE

class Ui_QcvWidget
{
public:
    QLabel *labelView;
    QTextBrowser *textBrowser;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QLCDNumber *lcdNumber;
    QPushButton *pushButtonPlay;
    QPushButton *realTimeMode;
    QTextBrowser *textUsedMode;
    QPushButton *OfflineMode;
    QLabel *skinDetected;
    QCustomPlot *customPlot;

    void setupUi(QWidget *QcvWidget)
    {
        if (QcvWidget->objectName().isEmpty())
            QcvWidget->setObjectName(QStringLiteral("QcvWidget"));
        QcvWidget->resize(1213, 763);
        labelView = new QLabel(QcvWidget);
        labelView->setObjectName(QStringLiteral("labelView"));
        labelView->setGeometry(QRect(40, 90, 451, 371));
        labelView->setStyleSheet(QStringLiteral("border: 1px solid;"));
        textBrowser = new QTextBrowser(QcvWidget);
        textBrowser->setObjectName(QStringLiteral("textBrowser"));
        textBrowser->setGeometry(QRect(620, 20, 171, 31));
        layoutWidget = new QWidget(QcvWidget);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(620, 70, 181, 211));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        lcdNumber = new QLCDNumber(layoutWidget);
        lcdNumber->setObjectName(QStringLiteral("lcdNumber"));

        verticalLayout->addWidget(lcdNumber);

        pushButtonPlay = new QPushButton(layoutWidget);
        pushButtonPlay->setObjectName(QStringLiteral("pushButtonPlay"));

        verticalLayout->addWidget(pushButtonPlay);

        realTimeMode = new QPushButton(QcvWidget);
        realTimeMode->setObjectName(QStringLiteral("realTimeMode"));
        realTimeMode->setGeometry(QRect(130, 50, 121, 21));
        realTimeMode->setContextMenuPolicy(Qt::ActionsContextMenu);
        textUsedMode = new QTextBrowser(QcvWidget);
        textUsedMode->setObjectName(QStringLiteral("textUsedMode"));
        textUsedMode->setGeometry(QRect(100, 10, 341, 31));
        OfflineMode = new QPushButton(QcvWidget);
        OfflineMode->setObjectName(QStringLiteral("OfflineMode"));
        OfflineMode->setGeometry(QRect(290, 50, 121, 21));
        OfflineMode->setContextMenuPolicy(Qt::ActionsContextMenu);
        skinDetected = new QLabel(QcvWidget);
        skinDetected->setObjectName(QStringLiteral("skinDetected"));
        skinDetected->setGeometry(QRect(620, 320, 291, 271));
        skinDetected->setStyleSheet(QStringLiteral("border: 1px solid;"));
        customPlot = new QCustomPlot(QcvWidget);
        customPlot->setObjectName(QStringLiteral("customPlot"));
        customPlot->setGeometry(QRect(810, 50, 391, 261));

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
        realTimeMode->setText(QApplication::translate("QcvWidget", "Real Time Video ", 0));
        textUsedMode->setHtml(QApplication::translate("QcvWidget", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Please Choose Mode before START</span></p></body></html>", 0));
        OfflineMode->setText(QApplication::translate("QcvWidget", "Offline Video", 0));
        skinDetected->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class QcvWidget: public Ui_QcvWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QCVWIDGET_H

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
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QWidget>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE

class Ui_QcvWidget
{
public:
    QLabel *labelView;
    QTextBrowser *textBrowser;
    QPushButton *realTimeMode;
    QTextBrowser *textUsedMode;
    QPushButton *OfflineMode;
    QLabel *skinDetected;
    QCustomPlot *customPlot;
    QTextBrowser *textBrowser_2;
    QCustomPlot *FFTOutput;
    QTextBrowser *textBrowser_3;
    QSpinBox *spinBox;
    QTextBrowser *textBrowser_4;
    QPushButton *pushButtonPlay;
    QLCDNumber *lcdNumber;
    QPushButton *pushButtonReset;
    QPushButton *pushButtonWinLenValidate;

    void setupUi(QWidget *QcvWidget)
    {
        if (QcvWidget->objectName().isEmpty())
            QcvWidget->setObjectName(QStringLiteral("QcvWidget"));
        QcvWidget->resize(1294, 763);
        labelView = new QLabel(QcvWidget);
        labelView->setObjectName(QStringLiteral("labelView"));
        labelView->setGeometry(QRect(40, 90, 501, 611));
        labelView->setStyleSheet(QStringLiteral("border: 1px solid;"));
        textBrowser = new QTextBrowser(QcvWidget);
        textBrowser->setObjectName(QStringLiteral("textBrowser"));
        textBrowser->setGeometry(QRect(610, 20, 171, 31));
        realTimeMode = new QPushButton(QcvWidget);
        realTimeMode->setObjectName(QStringLiteral("realTimeMode"));
        realTimeMode->setGeometry(QRect(150, 50, 121, 21));
        realTimeMode->setContextMenuPolicy(Qt::ActionsContextMenu);
        textUsedMode = new QTextBrowser(QcvWidget);
        textUsedMode->setObjectName(QStringLiteral("textUsedMode"));
        textUsedMode->setGeometry(QRect(120, 10, 341, 41));
        OfflineMode = new QPushButton(QcvWidget);
        OfflineMode->setObjectName(QStringLiteral("OfflineMode"));
        OfflineMode->setGeometry(QRect(310, 50, 121, 21));
        OfflineMode->setContextMenuPolicy(Qt::ActionsContextMenu);
        skinDetected = new QLabel(QcvWidget);
        skinDetected->setObjectName(QStringLiteral("skinDetected"));
        skinDetected->setGeometry(QRect(590, 340, 311, 361));
        skinDetected->setStyleSheet(QStringLiteral("border: 1px solid;"));
        customPlot = new QCustomPlot(QcvWidget);
        customPlot->setObjectName(QStringLiteral("customPlot"));
        customPlot->setGeometry(QRect(940, 40, 351, 311));
        textBrowser_2 = new QTextBrowser(QcvWidget);
        textBrowser_2->setObjectName(QStringLiteral("textBrowser_2"));
        textBrowser_2->setGeometry(QRect(1000, 0, 261, 31));
        FFTOutput = new QCustomPlot(QcvWidget);
        FFTOutput->setObjectName(QStringLiteral("FFTOutput"));
        FFTOutput->setGeometry(QRect(940, 410, 351, 291));
        textBrowser_3 = new QTextBrowser(QcvWidget);
        textBrowser_3->setObjectName(QStringLiteral("textBrowser_3"));
        textBrowser_3->setGeometry(QRect(1000, 360, 291, 31));
        spinBox = new QSpinBox(QcvWidget);
        spinBox->setObjectName(QStringLiteral("spinBox"));
        spinBox->setGeometry(QRect(800, 150, 61, 41));
        textBrowser_4 = new QTextBrowser(QcvWidget);
        textBrowser_4->setObjectName(QStringLiteral("textBrowser_4"));
        textBrowser_4->setGeometry(QRect(790, 100, 141, 41));
        pushButtonPlay = new QPushButton(QcvWidget);
        pushButtonPlay->setObjectName(QStringLiteral("pushButtonPlay"));
        pushButtonPlay->setGeometry(QRect(600, 260, 91, 21));
        lcdNumber = new QLCDNumber(QcvWidget);
        lcdNumber->setObjectName(QStringLiteral("lcdNumber"));
        lcdNumber->setGeometry(QRect(600, 74, 181, 181));
        pushButtonReset = new QPushButton(QcvWidget);
        pushButtonReset->setObjectName(QStringLiteral("pushButtonReset"));
        pushButtonReset->setGeometry(QRect(690, 260, 91, 21));
        pushButtonWinLenValidate = new QPushButton(QcvWidget);
        pushButtonWinLenValidate->setObjectName(QStringLiteral("pushButtonWinLenValidate"));
        pushButtonWinLenValidate->setGeometry(QRect(870, 150, 61, 41));
        labelView->raise();
        textBrowser->raise();
        realTimeMode->raise();
        textUsedMode->raise();
        OfflineMode->raise();
        skinDetected->raise();
        customPlot->raise();
        textBrowser_2->raise();
        FFTOutput->raise();
        textBrowser_3->raise();
        lcdNumber->raise();
        spinBox->raise();
        textBrowser_4->raise();
        pushButtonPlay->raise();
        lcdNumber->raise();
        pushButtonReset->raise();
        pushButtonWinLenValidate->raise();

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
        realTimeMode->setText(QApplication::translate("QcvWidget", "Real Time Video ", 0));
        textUsedMode->setHtml(QApplication::translate("QcvWidget", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Please Choose Mode before START</span></p></body></html>", 0));
        OfflineMode->setText(QApplication::translate("QcvWidget", "Offline Video", 0));
        skinDetected->setText(QString());
        textBrowser_2->setHtml(QApplication::translate("QcvWidget", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Signal from Face Skin Detection</span></p></body></html>", 0));
        textBrowser_3->setHtml(QApplication::translate("QcvWidget", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">FFT from Face Skin Detection Signal</span></p></body></html>", 0));
        textBrowser_4->setHtml(QApplication::translate("QcvWidget", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Window duration in s for processing</span></p></body></html>", 0));
        pushButtonPlay->setText(QApplication::translate("QcvWidget", "start", 0));
        pushButtonReset->setText(QApplication::translate("QcvWidget", "reset", 0));
        pushButtonWinLenValidate->setText(QApplication::translate("QcvWidget", "OK", 0));
    } // retranslateUi

};

namespace Ui {
    class QcvWidget: public Ui_QcvWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QCVWIDGET_H

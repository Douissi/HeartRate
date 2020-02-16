#include "qcvwidget.h"
#include "ui_qcvwidget.h"

QcvWidget::QcvWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QcvWidget)
{
    ui->setupUi(this);
}

QcvWidget::~QcvWidget()
{
    delete ui;
}

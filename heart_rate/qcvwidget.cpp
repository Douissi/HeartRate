#include "qcvwidget.h"
#include "ui_qcvwidget.h"
#include "videoProcessing.hpp"
#include <QTimer>

QcvWidget::QcvWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QcvWidget)
{
    ui->setupUi(this);
    ui->labelView->setScaledContents(true);
    setup();
}

QcvWidget::~QcvWidget()
{
    thread->quit();
    while(!thread->isFinished()); // thread->wait()

    delete thread;
    delete ui;
}

void QcvWidget::setup()
{
    thread = new QThread();
    IvideoProcessing* worker = new IvideoProcessing();
    QTimer* workerTrigger = new QTimer();
    workerTrigger->setInterval(static_cast<int>((1.0/worker->getFs())*1000.0)); // maybe 1/Fs = 33 ms of the video ???
    ui->labelView->setFixedWidth(worker->getWidth());
    ui->labelView->setFixedHeight(worker->getHeight());

    connect(workerTrigger, SIGNAL(timeout()), worker, SLOT(receiveGrabFrame()));
    connect(this, SIGNAL(sendToggleSTream()), worker, SLOT(receiveToggleStream()));
    connect(ui->pushButtonPlay, SIGNAL(clicked(bool)), this, SLOT(receiveToggleStream()));
    connect(worker, SIGNAL(sendFrame(const QImage&)), this, SLOT(receiveFrame(const QImage&)));
    connect(worker, SIGNAL(sendHeartRateEstimation(float)), this, SLOT(receiveHeartRateEstimation(float)));

    connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater())); // check how videoObj is deleted
    connect(thread, SIGNAL(finished()), workerTrigger, SLOT(deleteLater()));

    workerTrigger->start();
    worker->moveToThread(thread);
    workerTrigger->moveToThread(thread);

    thread->start();
}


void QcvWidget::receiveFrame(const QImage& frame)
{
    ui->labelView->setPixmap(QPixmap::fromImage(frame));
}

void QcvWidget::receiveHeartRateEstimation(const float& heartRate)
{
    ui->lcdNumber->display(static_cast<double>(heartRate));
}

void QcvWidget::receiveToggleStream()
{
    if(!ui->pushButtonPlay->text().compare("start")) ui->pushButtonPlay->setText("stop");
    else ui->pushButtonPlay->setText("start");

    emit sendToggleSTream();
}

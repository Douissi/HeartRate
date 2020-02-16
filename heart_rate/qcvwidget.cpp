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
    workerTrigger->setInterval(33); // maybe 1/Fs of the video ???


    connect(workerTrigger, SIGNAL(timeout()), worker, SLOT(receiveGrabFrame()));
    connect(this, SIGNAL(sendToggleSTream()), worker, SLOT(receiveToggleStream()));
    connect(ui->pushButtonPlay, SIGNAL(clicked(bool)), this, SLOT(receiveToggleStream()));
    connect(worker, SIGNAL(sendFrame(QImage)), this, SLOT(receiveFrame(QImage)));

    connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater())); // check how videoObj is deleted
    connect(thread, SIGNAL(finished()), workerTrigger, SLOT(deleteLater()));

    workerTrigger->start();
    worker->moveToThread(thread);
    workerTrigger->moveToThread(thread);

    thread->start();
}


void QcvWidget::receiveFrame(QImage frame)
{
    ui->labelView->setPixmap(QPixmap::fromImage(frame));
}

void QcvWidget::receiveToggleStream()
{
    if(!ui->pushButtonPlay->text().compare("start")) ui->pushButtonPlay->setText("stop");
    else ui->pushButtonPlay->setText("start");

    emit sendToggleSTream();
}

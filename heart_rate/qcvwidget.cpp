#include "qcvwidget.h"
#include "ui_qcvwidget.h"
#include "videoProcessing.hpp"
#include <QTimer>

QcvWidget::QcvWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QcvWidget),
    Fs(0.0)
{
    ui->setupUi(this);
    ui->labelView->setScaledContents(true);
    qRegisterMetaType<vectorFloat>("vectorFloat");
    qRegisterMetaType<string>("string");
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
    IvideoProcessing* worker = new IvideoProcessing("face.mp4");
    QTimer* workerTrigger = new QTimer();
    Fs = worker->getFs();
    workerTrigger->setInterval(static_cast<int>((1.0/Fs)*1000.0)); // maybe 1/Fs = 33 ms of the video ???
    configureUi();

    connect(workerTrigger, SIGNAL(timeout()), worker, SLOT(receiveGrabFrame()));
    connect(this, SIGNAL(sendToggleSTream()), worker, SLOT(receiveToggleStream()));
    connect(ui->pushButtonPlay, SIGNAL(clicked(bool)), this, SLOT(receiveToggleStream()));
    connect(ui->realTimeMode, SIGNAL(clicked(bool)), this, SLOT(receiveRealTimeMode()));
    connect(ui->OfflineMode, SIGNAL(clicked(bool)), this, SLOT(receiveOfflineMode()));
    connect(worker, SIGNAL(sendFrame(const QImage)), this, SLOT(receiveFrame(const QImage)));
    connect(worker, SIGNAL(sendFace(const QImage)), this, SLOT(receiveFace(const QImage)));
    connect(worker, SIGNAL(sendHeartRateEstimation(float)), this, SLOT(receiveHeartRateEstimation(float)));
    connect(worker, SIGNAL(sendSignalToPlot(const vectorFloat)), this, SLOT(faceSignalPlot(const vectorFloat)));
    connect(worker, SIGNAL(sendFFTtoPlot(const vectorFloat)), this, SLOT(faceFFTPlot(const vectorFloat)));
    connect(this, SIGNAL(sendStopStream()), worker, SLOT(receiveStopStream()));
    connect(this, SIGNAL(sendReconstructOfflineVideoObj(const string)), worker, SLOT(receiveReconstructVideoObj(const string)));

    connect(thread, SIGNAL(finished()), worker, SLOT(deleteLater())); // check how videoObj is deleted
    connect(thread, SIGNAL(finished()), workerTrigger, SLOT(deleteLater()));

    workerTrigger->start();
    worker->moveToThread(thread);
    workerTrigger->moveToThread(thread);

    thread->start();
}

void QcvWidget::configureUi()
{
//    ui->labelView->setFixedWidth(worker->getWidth());
//    ui->labelView->setFixedHeight(worker->getHeight());
    ui->labelView->hasScaledContents();
    ui->labelView->setScaledContents(true);
    ui->skinDetected->hasScaledContents();
    ui->skinDetected->setScaledContents(true);
    ui->textUsedMode->setFontWeight(QFont::Bold);
}


void QcvWidget::receiveFrame(const QImage frame)
{
    ui->labelView->setPixmap(QPixmap::fromImage(frame));
}

void QcvWidget::receiveFace(const QImage face)
{
    ui->skinDetected->setPixmap(QPixmap::fromImage(face));
}

void QcvWidget::receiveHeartRateEstimation(const float heartRate)
{
    ui->lcdNumber->display(static_cast<double>(heartRate));
}

void QcvWidget::receiveToggleStream()
{
    if(!ui->pushButtonPlay->text().compare("start")) ui->pushButtonPlay->setText("stop");
    else ui->pushButtonPlay->setText("start");

    emit sendToggleSTream();
}

void QcvWidget::faceSignalPlot(const vectorFloat sigToPlot)
{
    const unsigned int nbOfSamples = sigToPlot.size();
    // generate some data:
    QVector<double> x(nbOfSamples), y(nbOfSamples);
    for (unsigned int i=0; i < nbOfSamples; ++i)
    {
      x[i] = i;
      y[i] = sigToPlot[i];
    }
    // create graph and assign data to it:
    ui->customPlot->addGraph();
    ui->customPlot->graph(0)->setData(x, y);
    // give the axes some labels:
    ui->customPlot->xAxis->setLabel("Signal extracted from Face Skin detection");
    ui->customPlot->yAxis->setLabel("Amplitude of the Signal");
    // set axes ranges, so we see all data:
    ui->customPlot->xAxis->setRange(0, nbOfSamples);
    ui->customPlot->yAxis->setRange(*min_element(sigToPlot.begin(), sigToPlot.end()),
                                    *max_element(sigToPlot.begin(), sigToPlot.end()));
    ui->customPlot->replot();
}

void QcvWidget::faceFFTPlot(const vectorFloat FFTtoPlot)
{
    const unsigned int nbOfSamples = FFTtoPlot.size();
    // generate some data:
    QVector<double> x(nbOfSamples), y(nbOfSamples);
    for (unsigned int i=0; i < nbOfSamples; ++i)
    {
      x[i] = static_cast<double>(i) * Fs / (2.0 * static_cast<double>(nbOfSamples)); // 30 = Fech
      y[i] = FFTtoPlot[i];
    }
    // create graph and assign data to it:
    ui->FFTOutput->addGraph();
    ui->FFTOutput->graph(0)->setData(x, y);
    // give the axes some labels:
    ui->FFTOutput->xAxis->setLabel("FFT extracted from Face Skin detection");
    ui->FFTOutput->yAxis->setLabel("Amplitude of the FFT");
    // set axes ranges, so we see all data:
    ui->FFTOutput->xAxis->setRange(0, 6); // Max of Heart Rate freq not exceeding 6 Hz
    ui->FFTOutput->yAxis->setRange(*min_element(FFTtoPlot.begin(), FFTtoPlot.end()),
                                    *max_element(FFTtoPlot.begin(), FFTtoPlot.end()));
    ui->FFTOutput->replot();
}

void QcvWidget::receiveOfflineMode()
{
    ui->textUsedMode->setText("Offline Mode Used, click start to begin");
    ui->pushButtonPlay->setText("start");
    emit sendStopStream();
    QString s1 = QFileDialog::getOpenFileName(this, "Open a file", "directoryToOpen",
                                              "Videos (*.mp4 *.wmv)");
    emit sendReconstructOfflineVideoObj(s1.toUtf8().constData());
}

void QcvWidget::receiveRealTimeMode()
{
   ui->textUsedMode->setText("Real Time Mode Used, click start to begin");
}

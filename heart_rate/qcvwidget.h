#ifndef QCVWIDGET_H
#define QCVWIDGET_H

#include <QWidget>
#include <QThread>
#include "videoProcessing.hpp"


namespace Ui {
class QcvWidget;
}

class QcvWidget : public QWidget
{
    Q_OBJECT

private:
    Ui::QcvWidget *ui;
    QThread* thread;
    double Fs;
    int windowLen;

    void setup();
    void configureUi();

public:
    explicit QcvWidget(QWidget *parent = 0);
    ~QcvWidget();

signals:
    void sendToggleSTream();
    void sendStopStream();
    void sendResetStream();
    void sendWindowLenChange(const int windowLen);
    void sendReconstructOfflineVideoObj(const string input);

private slots:
    void receiveFrame(const QImage frame);
    void receiveFace(const QImage face);
    void receiveHeartRateEstimation(const float heartRate);
    void receiveToggleStream();
    void receiveRealTimeMode();
    void receiveOfflineMode();
    void receiveResetStream();
    void receiveWinLenChange();
    void faceSignalPlot(const vectorFloat sigToPlot);
    void faceFFTPlot(const vectorFloat FFTtoPlot);

};

#endif // QCVWIDGET_H

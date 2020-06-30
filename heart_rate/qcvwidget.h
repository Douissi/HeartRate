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

    void setup();
    void configureUi(const IvideoProcessing* const worker);

public:
    explicit QcvWidget(QWidget *parent = 0);
    ~QcvWidget();

signals:
    void sendToggleSTream();

private slots:
    void receiveFrame(const QImage& frame);
    void receiveFace(const QImage& face);
    void receiveHeartRateEstimation(const float& heartRate);
    void receiveToggleStream();
    void receiveRealTimeMode();
    void receiveOfflineMode();

    void makePlot();
};

#endif // QCVWIDGET_H

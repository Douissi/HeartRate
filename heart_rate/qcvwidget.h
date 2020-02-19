#ifndef QCVWIDGET_H
#define QCVWIDGET_H

#include <QWidget>
#include <QThread>

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

public:
    explicit QcvWidget(QWidget *parent = 0);
    ~QcvWidget();

signals:
    void sendToggleSTream();

private slots:
    void receiveFrame(const QImage& frame);
    void receiveHeartRateEstimation(const float& heartRate);
    void receiveToggleStream();

};

#endif // QCVWIDGET_H

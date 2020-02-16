#ifndef QCVWIDGET_H
#define QCVWIDGET_H

#include <QWidget>

namespace Ui {
class QcvWidget;
}

class QcvWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QcvWidget(QWidget *parent = 0);
    ~QcvWidget();

private:
    Ui::QcvWidget *ui;
};

#endif // QCVWIDGET_H

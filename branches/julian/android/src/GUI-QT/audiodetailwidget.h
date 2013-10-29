#ifndef AUDIODETAILWIDGET_H
#define AUDIODETAILWIDGET_H

#include <QWidget>
#include "../Parameter.h"

namespace Ui {
class AudioDetailWidget;
}

class AudioDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AudioDetailWidget(QWidget *parent = 0);
    ~AudioDetailWidget();
    void updateDisplay(int, const CService&);

signals:
    void listen(int);
private:
    Ui::AudioDetailWidget *ui;
    int short_id;
    void addItem(const QString&, const QString&);
private slots:
    void on_buttonListen_clicked();
};

#endif // AUDIODETAILWIDGET_H

#ifndef STATIONSWIDGET_H
#define STATIONSWIDGET_H

#include <QWidget>
#include "StationsDlg.h"

namespace Ui {
class StationsWidget;
}

class ReceiverController;

class StationsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StationsWidget(ReceiverController*, QWidget *parent = 0);
    ~StationsWidget();

signals:
    void frequencyChanged(int);

public slots:
    void OnSwitchMode(int);
    void SetFrequency(int);

private:
    Ui::StationsWidget* ui;
    ERecMode eRecMode;
    CSchedule schedule;
    QString			strColumnParamdrm;
    QString			strColumnParamanalog;

    void			ColumnParamFromStr(const QString& strColumnParam);
    void			ColumnParamToStr(QString& strColumnParam);

private slots:
    void on_frequency_valueChanged(int);
};

#endif // STATIONSWIDGET_H

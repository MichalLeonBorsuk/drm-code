#ifndef AUDIODETAILWIDGET_H
#define AUDIODETAILWIDGET_H

#include <QWidget>
#include "../Parameter.h"

namespace Ui {
class AudioDetailWidget;
}

class CDRMPlot;

class AudioDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AudioDetailWidget(QString, CDRMReceiver*, QWidget * = 0);
    ~AudioDetailWidget();
    void updateDisplay(int, const CService&);
    void setEngineering(bool);

signals:
    void listen(int);
public slots:
    void setPlotStyle(int);
private:
    Ui::AudioDetailWidget *ui;
    int short_id;
    QString description;
    bool engineeringMode;
    CDRMPlot *pMainPlot;
    CDRMReceiver* pDRMReceiver;
    int iPlotStyle;
    void updateEngineeringModeDisplay(int, const CService&);
    void updateUserModeDisplay(int, const CService&);
    void addItem(const QString&, const QString&);
private slots:
    void on_buttonListen_clicked();
    void on_mute_stateChanged(int);
    void on_reverb_stateChanged(int);
    void on_save_stateChanged(int);
};

#endif // AUDIODETAILWIDGET_H

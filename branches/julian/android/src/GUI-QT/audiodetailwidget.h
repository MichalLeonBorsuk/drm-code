#ifndef AUDIODETAILWIDGET_H
#define AUDIODETAILWIDGET_H

#include <QWidget>
#include "../Parameter.h"

namespace Ui {
class AudioDetailWidget;
}

class CDRMPlot;
#if QT_VERSION>=0x050100
class QQuickView;
class QQuickItem;
#endif

class AudioDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AudioDetailWidget(CDRMReceiver*, QWidget * = 0);
    ~AudioDetailWidget();
    void updateDisplay(int, const CService&);
    void setEngineering(bool);

signals:
    void listen(int);
public slots:
    void setPlotStyle(int);
    void setDescription(const QString&);
private:
    Ui::AudioDetailWidget *ui;
    int short_id;
    bool engineeringMode;
    CDRMPlot *pMainPlot;
    CDRMReceiver* pDRMReceiver;
    int iPlotStyle;
#if QT_VERSION>=0x050100
    QQuickView *view;
    QQuickItem* userModeDisplay;
#endif
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

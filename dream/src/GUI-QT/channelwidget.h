#ifndef CHANNELWIDGET_H
#define CHANNELWIDGET_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <../Parameter.h>
#include <../chanest/ChannelEstimation.h>
#include <../chanest/ChanEstTime.h>

class CDRMPlot;

namespace Ui {
class ChannelWidget;
}

class ChannelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChannelWidget(CDRMReceiver*, QWidget *parent = 0);
    ~ChannelWidget();

private:
    Ui::ChannelWidget *ui;
    CDRMPlot *pMainPlot;
    CDRMReceiver* pDRMReceiver;
    int iPlotStyle;
public slots:
    void setActive(bool);
    void setLEDFAC(ETypeRxStatus);
    void on_DRMMainWindow_SDCChanged(ETypeRxStatus status);
    void setLEDFrameSync(ETypeRxStatus status);
    void setLEDTimeSync(ETypeRxStatus status);
    void setLEDIOInterface(ETypeRxStatus status);
    void setSNR(double rSNR);
    void setMER(double rMER, double rWMERMSC);
    void setDelay_Doppler(double rSigmaEstimate, double rMinDelay);
    void setSampleFrequencyOffset(double rCurSamROffs, double rSampleRate);
    void setFrequencyOffset(double);
    void setChannel(ERobMode, ESpecOcc, ESymIntMod, ECodScheme, ECodScheme);
    void setCodeRate(int,int);
    void setPlotStyle(int);
    void setNumIterations(int);
    void setTimeInt(CChannelEstimation::ETypeIntTime);
    void setFreqInt(CChannelEstimation::ETypeIntFreq);
    void setTiSyncTrac(CTimeSyncTrack::ETypeTiSyncTrac);
    void setRecFilterEnabled(bool);
    void setIntConsEnabled(bool);
    void setFlipSpectrumEnabled(bool);

private slots:
    void on_chartSelector_currentItemChanged(QTreeWidgetItem *);
    void on_showOptions_toggled(bool);

signals:
    void noOfIterationsChanged(int);
    void TimeLinear();
    void TimeWiener();
    void FrequencyLinear();
    void FrequencyDft();
    void FrequencyWiener();
    void TiSyncEnergy();
    void TiSyncFirstPeak();
    void FlipSpectrum(int);
    void RecFilter(int);
    void ModiMetric(int);
};

#endif // ChannelWidget_H

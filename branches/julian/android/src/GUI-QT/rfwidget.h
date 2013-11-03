#ifndef RFWIDGET_H
#define RFWIDGET_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <../Parameter.h>
class CDRMPlot;

namespace Ui {
class RFWidget;
}

class RFWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RFWidget(CDRMReceiver*, QWidget *parent = 0);
    ~RFWidget();

private:
    Ui::RFWidget *ui;
    CDRMPlot *pMainPlot;
    CDRMReceiver* pDRMReceiver;
public slots:
    void setActive(bool);
    void setLEDFAC(ETypeRxStatus);
    void setLEDSDC(ETypeRxStatus status);
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
private slots:
    void on_chartSelector_currentItemChanged(QTreeWidgetItem *);

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

#endif // RFWIDGET_H

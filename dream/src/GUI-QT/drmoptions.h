#ifndef DRMOPTIONS_H
#define DRMOPTIONS_H

#include <QWidget>
#include <../chanest/ChannelEstimation.h>
#include <../sync/TimeSyncTrack.h>

namespace Ui {
class DRMOptions;
}

class DRMOptions : public QWidget
{
    Q_OBJECT

public:
    explicit DRMOptions(QWidget *parent = 0);
    ~DRMOptions();
    void setRSCIModeEnabled(bool enabled);
    void setNumIterations(int iNumIt);
    void setTimeInt(CChannelEstimation::ETypeIntTime);
    void setFreqInt(CChannelEstimation::ETypeIntFreq);
    void setTiSyncTrac(CTimeSyncTrack::ETypeTiSyncTrac);
    void setRecFilterEnabled(bool);
    void setIntConsEnabled(bool);
    void setFlipSpectrumEnabled(bool);

private:
    Ui::DRMOptions *ui;
    bool rsci_mode;
    void AddWhatsThisHelp();

private slots:
    void on_SliderNoOfIterations_valueChanged(int);

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

#endif // DRMOPTIONS_H

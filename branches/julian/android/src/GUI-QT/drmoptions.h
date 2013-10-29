#ifndef DRMOPTIONS_H
#define DRMOPTIONS_H

#include <QWidget>
#include <../DrmReceiver.h>

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
    void UpdateControls(CDRMReceiver& DRMReceiver);

private:
    Ui::DRMOptions *ui;
    void AddWhatsThisHelp();

private slots:
    void on_change_SliderNoOfIterations(int);

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

#ifndef DRMDETAIL_H
#define DRMDETAIL_H

#include <QWidget>
#include <../Parameter.h>
#include "MultColorLED.h"

namespace Ui {
class DRMDetail;
}

class DRMDetail : public QWidget
{
    Q_OBJECT

public:
    explicit DRMDetail(QWidget *parent = 0);
    ~DRMDetail();
    void updateDisplay(CParameter& Parameters, _REAL freqOffset, EAcqStat acqState, bool rsciMode);

private:
    Ui::DRMDetail *ui;
    QString	GetRobModeStr(ERobMode e);
    QString	GetSpecOccStr(ESpecOcc e);
    void AddWhatsThisHelp();
};

#endif // DRMDETAIL_H

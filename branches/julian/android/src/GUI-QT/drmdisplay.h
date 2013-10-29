#ifndef DRMDISPLAY_H
#define DRMDISPLAY_H

#include <QWidget>
#include <../Parameter.h>
#include <MultColorLED.h>

namespace Ui {
class DRMDisplay;
}

class DRMDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit DRMDisplay(QWidget *parent = 0);
    ~DRMDisplay();
    void setBars(int);
    void setLevel(_REAL);
    void showReceptionStatus(ETypeRxStatus fac, ETypeRxStatus sdc, ETypeRxStatus msc);
    void showTextMessage(const QString&);
    void showServiceInfo(const CService&);
    void setBitRate(_REAL rBitRate, _REAL rPartABLenRat);
    void clearDisplay(const QString& serviceLabel);
    void SetDisplayColor(const QColor&);

private:
    Ui::DRMDisplay *ui;
    void AddWhatsThisHelp();
};

#endif // DRMDISPLAY_H

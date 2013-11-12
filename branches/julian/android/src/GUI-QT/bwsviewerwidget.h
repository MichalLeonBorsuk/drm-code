#ifndef BWSVIEWER_H
#define BWSVIEWER_H

#include <QWidget>
#include <BWSViewer.h>

class CDRMReceiver;
class CMOTDABDec;
class QShowEvent;
class QHideEvent;

class BWSViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BWSViewerWidget(CMOTDABDec*, CSettings&, QWidget* parent = 0);
    ~BWSViewerWidget();
public slots:
    void setServiceInfo(CService s) { viewer.setServiceInfo(s); }
    void setRxStatus(int, int, ETypeRxStatus);
private:
    BWSViewer viewer;
    void showEvent(QShowEvent*);
    void hideEvent(QShowEvent*);
};

#endif // BWSVIEWER_H

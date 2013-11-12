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
    explicit BWSViewerWidget(CDRMReceiver&, CMOTDABDec*, CSettings&, int, QWidget* parent = 0);
    ~BWSViewerWidget();

private:
    BWSViewer viewer;
    void showEvent(QShowEvent*);
    void hideEvent(QShowEvent*);
};

#endif // BWSVIEWER_H

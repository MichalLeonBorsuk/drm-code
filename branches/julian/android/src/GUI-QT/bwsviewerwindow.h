#ifndef BWSVIEWERWINDOW_H
#define BWSVIEWERWINDOW_H

#include "CWindow.h"
#include "BWSViewer.h"

class CDRMReceiver;
class CMOTDABDec;
class QWidget;

class BWSViewerWindow : public CWindow
{
    Q_OBJECT

public:
    explicit BWSViewerWindow(CDRMReceiver&, CMOTDABDec*, CSettings&, int, QWidget* parent = 0);
    virtual ~BWSViewerWindow();
private:
    BWSViewer viewer;

    void showEvent(QShowEvent*);
    void hideEvent(QShowEvent*);
};

#endif // BWSVIEWERWINDOW_H

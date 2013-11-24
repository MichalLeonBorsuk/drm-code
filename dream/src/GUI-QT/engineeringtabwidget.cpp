#include "engineeringtabwidget.h"
#include "channelwidget.h"
#include <QLabel>

EngineeringTabWidget::EngineeringTabWidget(CDRMReceiver* rx, QWidget *parent) :
    QTabWidget(parent)
{
    int iPlotStyle = 0;// TODO set from menu
    ChannelWidget* pCh = new ChannelWidget(rx);
    pCh->setPlotStyle(iPlotStyle);
    connect(this, SIGNAL(plotStyleChanged(int)), pCh, SLOT(setPlotStyle(int)));

    addTab(pCh, "Channel");
    addTab(new QLabel("Streams"), "Streams");
    addTab(new QLabel("AFS"), "AFS");
    addTab(new QLabel("GPS"), "GPS");

}
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
void setPlotStyle(int);
void setNumIterations(int);
void setTimeInt(CChannelEstimation::ETypeIntTime);
void setFreqInt(CChannelEstimation::ETypeIntFreq);
void setTiSyncTrac(CTimeSyncTrack::ETypeTiSyncTrac);
void setRecFilterEnabled(bool);
void setIntConsEnabled(bool);
void setFlipSpectrumEnabled(bool);

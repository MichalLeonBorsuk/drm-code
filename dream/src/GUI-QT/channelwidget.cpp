#include "channelwidget.h"
#include "ui_channelwidget.h"
#include <QTabBar>
#include "cdrmplotqwt.h"

ChannelWidget::ChannelWidget(ReceiverController* c, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChannelWidget),
    pMainPlot(NULL)
{
    ui->setupUi(this);
    pMainPlot = new CDRMPlotQwt(NULL, ui->plot, c); // must be after setupUi
    pMainPlot->setupTreeWidget(ui->chartSelector);
    pMainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD);
    ui->drmDetail->hideMSCParams(true);
    connectController(c);
}

ChannelWidget::~ChannelWidget()
{
    delete ui;
}

void ChannelWidget::connectController(ReceiverController* controller)
{
    // Display
    connect(controller, SIGNAL(MSCChanged(ETypeRxStatus)), ui->drmDetail, SLOT(setLEDMSC(ETypeRxStatus)));
    connect(controller, SIGNAL(SDCChanged(ETypeRxStatus)), ui->drmDetail, SLOT(setLEDSDC(ETypeRxStatus)));
    connect(controller, SIGNAL(FACChanged(ETypeRxStatus)), ui->drmDetail, SLOT(setLEDFAC(ETypeRxStatus)));
    connect(controller, SIGNAL(FSyncChanged(ETypeRxStatus)), ui->drmDetail, SLOT(setLEDFrameSync(ETypeRxStatus)));
    connect(controller, SIGNAL(TSyncChanged(ETypeRxStatus)), ui->drmDetail, SLOT(setLEDTimeSync(ETypeRxStatus)));
    connect(controller, SIGNAL(InputStatusChanged(ETypeRxStatus)), ui->drmDetail, SLOT(setLEDIOInterface(ETypeRxStatus)));
    connect(controller, SIGNAL(channelConfigurationChanged(ChannelConfiguration)), this, SLOT(on_channelConfigurationChanged(ChannelConfiguration)));
    connect(controller, SIGNAL(channelReceptionChanged(Reception)), this, SLOT(on_channelReceptionChanged(Reception)));

    // Controls
    connect(ui->drmOptions, SIGNAL(recFilter(bool)), controller, SLOT(setRecFilter(bool)));
    connect(ui->drmOptions, SIGNAL(flipSpectrum(bool)), controller, SLOT(setFlippedSpectrum(bool)));
    connect(ui->drmOptions, SIGNAL(modiMetric(bool)), controller, SLOT(setIntCons(bool)));
    connect(ui->drmOptions, SIGNAL(noOfIterationsChanged(int)), controller, SLOT(setNumMSCMLCIterations(int)));
    connect(ui->drmOptions, SIGNAL(timeIntChanged(int)), controller, SLOT(setTimeInt(int)));
    connect(ui->drmOptions, SIGNAL(freqIntChanged(int)), controller, SLOT(setFreqInt(int)));
    connect(ui->drmOptions, SIGNAL(timeSyncChanged(int)), controller, SLOT(setTiSyncTracType(int)));
    // Control revertives
    connect(controller, SIGNAL(numMSCMLCIterationsChanged(int)), this, SLOT(setNumIterations(int)));
    connect(controller, SIGNAL(timeIntChanged(int)), this, SLOT(setTimeInt(int)));
    connect(controller, SIGNAL(freqIntChanged(int)), this, SLOT(setFreqInt(int)));
    connect(controller, SIGNAL(tiSyncTracTypeChanged(int)), this, SLOT(setTiSyncTrac(int)));
    connect(controller, SIGNAL(recFilterChanged(bool)), this, SLOT(setRecFilterEnabled(bool)));
    connect(controller, SIGNAL(intConsChanged(bool)), this, SLOT(setIntConsEnabled(bool)));
    connect(controller, SIGNAL(flippedSpectrumChanged(bool)), this, SLOT(setFlipSpectrumEnabled(bool)));
}


void ChannelWidget::showEvent(QShowEvent*)
{
    on_chartSelector_currentItemChanged(ui->chartSelector->currentItem());
    pMainPlot->activate();
}

void ChannelWidget::hideEvent(QHideEvent*)
{
    pMainPlot->deactivate();
}

void ChannelWidget::on_showOptions_toggled(bool enabled)
{
    if(enabled)
        ui->drmOptions->show();
    else
        ui->drmOptions->hide();
}

void ChannelWidget::on_channelConfigurationChanged(ChannelConfiguration c)
{
    setChannel(ERobMode(c.robm), ESpecOcc(c.mode), ESymIntMod(c.interl),
               ECodScheme(c.sdcConst), ECodScheme(c.mscConst));
    setCodeRate(c.protLev.iPartA, c.protLev.iPartB);
}

void ChannelWidget::on_channelReceptionChanged(Reception r)
{
    setSNR(r.snr);
    setMER(r.mer, r.wmer);
    setDelay_Doppler(r.sigmaEstimate, r.minDelay);
    setSampleFrequencyOffset(r.sampleOffset, r.sampleRate);
    setFrequencyOffset(r.dcOffset);
}

void ChannelWidget::on_chartSelector_currentItemChanged(QTreeWidgetItem *curr)
{
    /* Make sure we have a non root item */
    if (curr && curr->parent())
    {
        /* Get chart type from selected item */
         CDRMPlot::ECharType eCurCharType = CDRMPlot::ECharType(curr->data(0, Qt::UserRole).toInt());
        /* Setup chart */
        pMainPlot->SetupChart(eCurCharType);
    }
}

void ChannelWidget::setLEDFAC(ETypeRxStatus status)
{
    ui->drmDetail->setLEDFAC(status);
}

void ChannelWidget::setLEDSDC(ETypeRxStatus status)
{
    ui->drmDetail->setLEDSDC(status);
}

void ChannelWidget::setLEDFrameSync(ETypeRxStatus status)
{
    ui->drmDetail->setLEDFrameSync(status);
}

void ChannelWidget::setLEDTimeSync(ETypeRxStatus status)
{
    ui->drmDetail->setLEDTimeSync(status);
}

void ChannelWidget::setLEDIOInterface(ETypeRxStatus status)
{
    ui->drmDetail->setLEDIOInterface(status);
}

void ChannelWidget::setSNR(double rSNR)
{
    ui->drmDetail->setSNR(rSNR);
}

void ChannelWidget::setMER(double rMER, double rWMERMSC)
{
    ui->drmDetail->setMER(rMER, rWMERMSC);
}

void ChannelWidget::setDelay_Doppler(double rSigmaEstimate, double rMinDelay)
{
    ui->drmDetail->setDelay_Doppler(rSigmaEstimate, rMinDelay);
}

void ChannelWidget::setSampleFrequencyOffset(double rCurSamROffs, double rSampleRate)
{
    ui->drmDetail->setSampleFrequencyOffset(rCurSamROffs, rSampleRate);
}

void ChannelWidget::setFrequencyOffset(double rOffset)
{
    ui->drmDetail->setFrequencyOffset(rOffset);
}

void ChannelWidget::setChannel(ERobMode robm, ESpecOcc specocc, ESymIntMod eSymbolInterlMode, ECodScheme eSDCCodingScheme, ECodScheme eMSCCodingScheme)
{
    ui->drmDetail->setChannel(robm, specocc, eSymbolInterlMode, eSDCCodingScheme, eMSCCodingScheme);
}

void ChannelWidget::setCodeRate(int b, int a)
{
    ui->drmDetail->setCodeRate(b, a);
}

void ChannelWidget::setNumIterations(int n)
{
    ui->drmOptions->setNumIterations(n);
}

void ChannelWidget::setTimeInt(int e)
{
    ui->drmOptions->setTimeInt(CChannelEstimation::ETypeIntTime(e));
}

void ChannelWidget::setFreqInt(int e)
{
    ui->drmOptions->setFreqInt(CChannelEstimation::ETypeIntFreq(e));
}

void ChannelWidget::setTiSyncTrac(int e)
{
    ui->drmOptions->setTiSyncTrac(CTimeSyncTrack::ETypeTiSyncTrac(e));
}

void ChannelWidget::setRecFilterEnabled(bool b)
{
    ui->drmOptions->setRecFilterEnabled(b);
}

void ChannelWidget::setIntConsEnabled(bool b)
{
    ui->drmOptions->setIntConsEnabled(b);
}

void ChannelWidget::setFlipSpectrumEnabled(bool b)
{
    ui->drmOptions->setFlipSpectrumEnabled(b);
}

void ChannelWidget::setPlotStyle(int n)
{
    pMainPlot->SetPlotStyle(n);
}

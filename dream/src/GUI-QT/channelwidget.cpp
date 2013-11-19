#include "channelwidget.h"
#include "ui_channelwidget.h"
#include <QTabBar>
#include "DRMPlot.h"

ChannelWidget::ChannelWidget(CDRMReceiver* prx, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChannelWidget),pMainPlot(NULL),pDRMReceiver(prx),iPlotStyle(0)
{
    ui->setupUi(this);
    pMainPlot = new CDRMPlot(NULL, ui->plot);
    pMainPlot->SetRecObj(pDRMReceiver);
    pMainPlot->setupTreeWidget(ui->chartSelector);
    pMainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD);
    ui->drmDetail->hideMSCParams(true);
}

ChannelWidget::~ChannelWidget()
{
    delete ui;
}

void ChannelWidget::setActive(bool active)
{
    if(active)
    {
        on_chartSelector_currentItemChanged(ui->chartSelector->currentItem());
        pMainPlot->activate();
    }
    else
        pMainPlot->deactivate();
}

void ChannelWidget::on_showOptions_toggled(bool enabled)
{
    if(enabled)
        ui->drmOptions->show();
    else
        ui->drmOptions->hide();
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
    ui->drmOptions->noOfIterationsChanged(n);
}

void ChannelWidget::setTimeInt(CChannelEstimation::ETypeIntTime e)
{
    ui->drmOptions->setTimeInt(e);
}

void ChannelWidget::setFreqInt(CChannelEstimation::ETypeIntFreq e)
{
    ui->drmOptions->setFreqInt(e);
}

void ChannelWidget::setTiSyncTrac(CTimeSyncTrack::ETypeTiSyncTrac e)
{
    ui->drmOptions->setTiSyncTrac(e);
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
    /* Save the new style */
    iPlotStyle = n;
    /* Update main plot window */
    if(pMainPlot)
        pMainPlot->SetPlotStyle(iPlotStyle);
}

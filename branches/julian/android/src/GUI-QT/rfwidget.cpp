#include "rfwidget.h"
#include "ui_rfwidget.h"
#include <QTabBar>
#include "DRMPlot.h"

RFWidget::RFWidget(CDRMReceiver* prx, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RFWidget),pMainPlot(NULL),pDRMReceiver(prx),iPlotStyle(0)
{
    ui->setupUi(this);
    pMainPlot = new CDRMPlot(NULL, ui->plot);
    pMainPlot->SetRecObj(pDRMReceiver);
    pMainPlot->setupTreeWidget(ui->chartSelector);
    pMainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD);
    ui->drmDetail->hideMSCParams(true);
}

RFWidget::~RFWidget()
{
    delete ui;
}

void RFWidget::setActive(bool active)
{
    if(active)
    {
        on_chartSelector_currentItemChanged(ui->chartSelector->currentItem());
        pMainPlot->activate();
    }
    else
        pMainPlot->deactivate();
}

void RFWidget::on_showOptions_toggled(bool enabled)
{
    if(enabled)
        ui->drmOptions->show();
    else
        ui->drmOptions->hide();
}

void RFWidget::on_chartSelector_currentItemChanged(QTreeWidgetItem *curr)
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

void RFWidget::setLEDFAC(ETypeRxStatus status)
{
    ui->drmDetail->setLEDFAC(status);
}

void RFWidget::setLEDSDC(ETypeRxStatus status)
{
    ui->drmDetail->setLEDSDC(status);
}

void RFWidget::setLEDFrameSync(ETypeRxStatus status)
{
    ui->drmDetail->setLEDFrameSync(status);
}

void RFWidget::setLEDTimeSync(ETypeRxStatus status)
{
    ui->drmDetail->setLEDTimeSync(status);
}

void RFWidget::setLEDIOInterface(ETypeRxStatus status)
{
    ui->drmDetail->setLEDIOInterface(status);
}

void RFWidget::setSNR(double rSNR)
{
    ui->drmDetail->setSNR(rSNR);
}

void RFWidget::setMER(double rMER, double rWMERMSC)
{
    ui->drmDetail->setMER(rMER, rWMERMSC);
}

void RFWidget::setDelay_Doppler(double rSigmaEstimate, double rMinDelay)
{
    ui->drmDetail->setDelay_Doppler(rSigmaEstimate, rMinDelay);
}

void RFWidget::setSampleFrequencyOffset(double rCurSamROffs, double rSampleRate)
{
    ui->drmDetail->setSampleFrequencyOffset(rCurSamROffs, rSampleRate);
}

void RFWidget::setFrequencyOffset(double rOffset)
{
    ui->drmDetail->setFrequencyOffset(rOffset);
}

void RFWidget::setChannel(ERobMode robm, ESpecOcc specocc, ESymIntMod eSymbolInterlMode, ECodScheme eSDCCodingScheme, ECodScheme eMSCCodingScheme)
{
    ui->drmDetail->setChannel(robm, specocc, eSymbolInterlMode, eSDCCodingScheme, eMSCCodingScheme);
}

void RFWidget::setCodeRate(int b, int a)
{
    ui->drmDetail->setCodeRate(b, a);
}

void RFWidget::setNumIterations(int n)
{
    ui->drmOptions->noOfIterationsChanged(n);
}

void RFWidget::setTimeInt(CChannelEstimation::ETypeIntTime e)
{
    ui->drmOptions->setTimeInt(e);
}

void RFWidget::setFreqInt(CChannelEstimation::ETypeIntFreq e)
{
    ui->drmOptions->setFreqInt(e);
}

void RFWidget::setTiSyncTrac(CTimeSyncTrack::ETypeTiSyncTrac e)
{
    ui->drmOptions->setTiSyncTrac(e);
}

void RFWidget::setRecFilterEnabled(bool b)
{
    ui->drmOptions->setRecFilterEnabled(b);
}

void RFWidget::setIntConsEnabled(bool b)
{
    ui->drmOptions->setIntConsEnabled(b);
}

void RFWidget::setFlipSpectrumEnabled(bool b)
{
    ui->drmOptions->setFlipSpectrumEnabled(b);
}

void RFWidget::setPlotStyle(int n)
{
    /* Save the new style */
    iPlotStyle = n;
    /* Update main plot window */
    if(pMainPlot)
        pMainPlot->SetPlotStyle(iPlotStyle);
}

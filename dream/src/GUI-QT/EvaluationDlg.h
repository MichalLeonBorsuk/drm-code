/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef __EVALUATIONDLG_H
#define __EVALUATIONDLG_H

#include "ui_systemevalDlgbase.h"
#include "CWindow.h"
#include "MultColorLED.h"
#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#include "../DrmReceiver.h"
#include "DRMPlot.h"
#include "chartdialog.h"

/* Definitions ****************************************************************/
/* Define this macro if you prefer the QT-type of displaying date and time */
#define GUI_QT_DATE_TIME_TYPE


/* Classes ********************************************************************/
class ReceiverController;
class Reception;
class ChannelConfiguration;
class DRMDetail;

class systemevalDlg : public CWindow
{
    Q_OBJECT

public:
    systemevalDlg(ReceiverController*, CSettings&, QWidget* parent = 0);
    virtual ~systemevalDlg();
    void            connectController(ReceiverController*);
    void enableLogging(bool enable)
    {
        ui->CheckBoxWriteLog->setChecked(enable);
    }

protected:
    Ui::SystemEvaluationWindow* ui;
    ReceiverController* controller;
    CParameter&     Parameters;
    DRMDetail*      detail;

    CDRMPlot*       MainPlot;

    virtual void    eventShow(QShowEvent* pEvent);
    virtual void    eventHide(QHideEvent* pEvent);
    void            UpdateGPS(CParameter&);
    void            setControls(CSettings& s);
    void            AddWhatsThisHelp();
    ChartDialog*    OpenChartWin(ECharType eNewType);
    QTreeWidgetItem* FindItemByECharType(ECharType eCharType);
    string          ECharTypeToPlotName(ECharType eCharType);
    ECharType       PlotNameToECharType(const string& PlotName);

    QString         GetRobModeStr(ERobMode);
    QString         GetSpecOccStr(ESpecOcc v);

    QMenu*          pTreeWidgetContextMenu;
    ECharType       eCurCharType, eNewCharType;
    int             iPlotStyle;
    vector<ChartDialog*>    vecpDRMPlots;

public slots:
    void OnRadioTimeLinear();
    void OnRadioTimeWiener();
    void OnRadioFrequencyLinear();
    void OnRadioFrequencyDft();
    void OnRadioFrequencyWiener();
    void OnRadioTiSyncFirstPeak();
    void OnRadioTiSyncEnergy();
    void OnSliderIterChange(int value);
    void OnCheckFlipSpectrum();
    void OnCheckBoxMuteAudio();
    void OnCheckBoxReverb();
    void OnCheckWriteLog(int);
    void OnCheckSaveAudioWAV();
    void OnCheckRecFilter();
    void OnCheckModiMetric();
    void OnListSelChanged(QTreeWidgetItem*, QTreeWidgetItem*);
    void OnTreeWidgetContMenu(bool);
    void OnCustomContextMenuRequested(const QPoint&);
    void OnDataAvailable();
    void UpdatePlotStyle(int);
    void onReception(Reception& r);
    void onChannel(ChannelConfiguration& c);
signals:
    void startLogging();
    void stopLogging();
    void saveAudio(const string&);
    void setTimeInt(int);
    void setFreqInt(int);
    void setTiSyncTracType(int);
    void setNumMSCMLCIterations(int);
    void setFlippedSpectrum(bool);
    void setReverbEffect(bool);
    void setRecFilter(bool);
    void setIntCons(bool);
    void muteAudio(bool);
    void dataAvailable(ReceiverController* rc);
};

#endif

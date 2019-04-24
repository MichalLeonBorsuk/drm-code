/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *  Volker Fischer, Andrew Murphy
 *
 * Description:
 *  See AnalogDemDlg.cpp
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *  - Additional widgets for displaying AMSS information
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


#include "ui_AMMainWindow.h"
#include "ui_AMSSDlgbase.h"
#include "CWindow.h"
#include "SoundCardSelMenu.h"
#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "../tables/TableAMSS.h"
#include <QTimer>
#include <QDialog>
#include <QButtonGroup>
# include "DRMPlot.h"

/* Definitions ****************************************************************/
/* Update time of PLL phase dial control */
#define PLL_PHASE_DIAL_UPDATE_TIME              100

namespace Ui {
class CAMSSDlgBase;
class AMMainWindow;
}

/* Classes ********************************************************************/

class PhaseGauge : public QWidget
{
    Q_OBJECT

public:
    PhaseGauge(QWidget* parent = 0):QWidget(parent),angle(0.0) {}
    void setValue(double d) {
        angle = d;
        update();
    }
    void paintEvent(QPaintEvent *);
private:
    double angle;
};

class ReceiverController;

/* AMSS dialog -------------------------------------------------------------- */
class CAMSSDlg : public CWindow
{
    Q_OBJECT

public:
    CAMSSDlg(CDRMReceiver&, CSettings&, QWidget* parent = 0);

protected:
    Ui::CAMSSDlgBase* ui;
    CDRMReceiver&   DRMReceiver;

    QTimer          Timer;
    QTimer          TimerPLLPhaseDial;
    PhaseGauge*     phaseGauge;

    void            AddWhatsThisHelp();
    virtual void    eventShow(QShowEvent*);
    virtual void    eventHide(QHideEvent*);

public slots:
    void OnTimer();
    void OnTimerPLLPhaseDial();
};


/* Analog demodulation dialog ----------------------------------------------- */
class AnalogDemDlg : public CWindow
{
    Q_OBJECT

public:
    AnalogDemDlg(ReceiverController*, CSettings&, CFileMenu*, CSoundCardSelMenu*,
                 QWidget* parent = 0);

protected:
    Ui::AMMainWindow*   ui;
    ReceiverController* controller;
    QTimer              TimerPLLPhaseDial;
    QTimer              TimerClose;
    CAMSSDlg            AMSSDlg;
    CFileMenu*          pFileMenu;
    CSoundCardSelMenu*  pSoundCardMenu;
    PhaseGauge*         phaseGauge;
    int                 subSampleCount;
    CDRMPlot*           MainPlot;

    void UpdateControls();
    void UpdateSliderBandwidth();
    void AddWhatsThisHelp();
    void initPhaseDial();
    virtual void eventClose(QCloseEvent* pEvent);
    virtual void eventShow(QShowEvent* pEvent);
    virtual void eventHide(QHideEvent* pEvent);
    virtual void eventUpdate();

public slots:
    void UpdatePlotStyle(int);
    void OnSampleRateChanged();
    void OnSoundFileChanged(CDRMReceiver::ESFStatus);
    void OnTimer();
    void OnTimerPLLPhaseDial();
    void OnTimerClose();
    void OnPlotClicked(double dVal);
    void OnSwitchToDRM();
    void OnSwitchToFM();
    void OnHelpAbout() {
        emit About();
    }
    void OnWhatsThis();
    void on_CheckBoxMuteAudio_clicked(bool);
    void on_CheckBoxSaveAudioWave_clicked(bool);
    void on_CheckBoxAutoFreqAcq_clicked(bool);
    void on_CheckBoxPLL_clicked(bool);
    void on_checkBoxWaterfall_toggled(bool);
    void on_ButtonFreqOffset_clicked(bool);
    void on_ButtonBandWidth_clicked(bool);
    void on_SpinBoxNoiRedLevel_valueChanged(int);
    void on_SliderBandwidth_valueChanged(int);
    void on_ButtonGroupDemodulation_buttonClicked(int);
    void on_ButtonGroupAGC_buttonClicked(int);
    void on_ButtonGroupNoiseReduction_buttonClicked(int);
    void on_new_data();

signals:
    void SwitchMode(int);
    void NewAMAcquisition();
    void ViewStationsDlg();
    void ViewLiveScheduleDlg();
    void Closed();
    void About();
};

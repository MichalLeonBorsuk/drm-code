/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy
 *
 * Description:
 *	See AnalogMainWindow.cpp
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- Additional widgets for displaying AMSS information
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


#include "DialogUtil.h"
#include "../ReceiverInterface.h"
#include "../tables/TableAMSS.h"
#include <QTimer>

#include "ui_AnalogMainWindow.h"
#include "ui_AMSSDlg.h"

/* Definitions ****************************************************************/
/* Update time of PLL phase dial control */
#define PLL_PHASE_DIAL_UPDATE_TIME				100


/* Classes ********************************************************************/
class ReceiverSettingsDlg;
class CSettings;
class CDRMPlot;

/* AMSS dialog -------------------------------------------------------------- */
class CAMSSDlg : public QDialog, public Ui_AMSSDlg
{
	Q_OBJECT

public:
	CAMSSDlg(AnalogReceiverInterface&, CSettings&, QWidget* parent = 0, Qt::WFlags f = 0);
	/* dummy assignment operator to help MSVC8 */
	CAMSSDlg& operator=(const CAMSSDlg&)
	{ throw "should not happen"; return *this;}

protected:
	AnalogReceiverInterface&	Receiver;
	CSettings&		Settings;

	QTimer			Timer;
	QTimer			TimerPLLPhaseDial;
	void			AddWhatsThisHelp();
	virtual void	hideEvent(QHideEvent* pEvent);
    virtual void	showEvent(QShowEvent* pEvent);

public slots:
	void OnTimer();
	void OnTimerPLLPhaseDial();
};



/* Analog demodulation dialog ----------------------------------------------- */
class AnalogMainWindow : public QMainWindow, public Ui_AnalogMainWindow
{
	Q_OBJECT

public:
	AnalogMainWindow(ReceiverInterface&, CSettings&, QWidget* parent = 0,
		Qt::WFlags f = 0);
	/* dummy assignment operator to help MSVC8 */
	AnalogMainWindow& operator=(const AnalogMainWindow&)
	{ throw "should not happen"; return *this;}

	void 			UpdatePlotStyle();

protected:
    ReceiverInterface&      Receiver;
    CSettings&              Settings;
    ReceiverSettingsDlg*    pReceiverSettingsDlg;
    QDialog*                stationsDlg;
    QDialog*                liveScheduleDlg;
    CDRMPlot*               plot;

    QTimer                  Timer;
    QTimer                  TimerPLLPhaseDial;
    CAMSSDlg                AMSSDlg;
    bool                    quitWanted;
    QButtonGroup	    bgDemod, bgAGC, bgNoiseRed;

    void                    UpdateControls();
    void                    AddWhatsThisHelp();
    void                    showEvent(QShowEvent* pEvent);
    void                    hideEvent(QHideEvent* pEvent);
    void                    closeEvent(QCloseEvent* pEvent);

public slots:
    void OnTimer();
    void OnTimerPLLPhaseDial();
    void OnRadioDemodulation(int iID);
    void OnRadioAGC(int iID);
    void OnCheckBoxMuteAudio();
    void OnCheckBoxReverb();
    void OnCheckSaveAudioWAV();
    void OnCheckAutoFreqAcq();
    void OnCheckPLL();
    void OnChartxAxisValSet(double dVal);
    void OnSliderBWChange(int value);
    void OnRadioNoiRed(int iID);
    void OnNewAMAcquisition();
    void OnButtonWaterfall();
    void OnButtonAMSS();
    void OnSwitchToDRM();
    void OnHelpWhatsThis();
};

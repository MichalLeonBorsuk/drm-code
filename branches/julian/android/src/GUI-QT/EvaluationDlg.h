/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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
#include "DRMPlot.h"
#include "DialogUtil.h"
#include "MultColorLED.h"
#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#include "../DrmReceiver.h"

/* Definitions ****************************************************************/
/* Define this macro if you prefer the QT-type of displaying date and time */
#define GUI_QT_DATE_TIME_TYPE


/* Classes ********************************************************************/

class systemevalDlg : public CWindow
{
	Q_OBJECT

public:
	systemevalDlg(CDRMReceiver&, CSettings&, QWidget* parent = 0);
	virtual ~systemevalDlg();

	void SetStatus(CMultColorLED*, ETypeRxStatus);

protected:
    Ui::SystemEvaluationWindow *ui;
    CDRMReceiver&	DRMReceiver;

	QTimer			Timer;
	CDRMPlot*		MainPlot;

	virtual void	eventShow(QShowEvent* pEvent);
	virtual void	eventHide(QHideEvent* pEvent);
	void			UpdateGPS(CParameter&);
    void            UpdateControls();
	void			AddWhatsThisHelp();
	CDRMPlot*		OpenChartWin(CDRMPlot::ECharType eNewType);
	QTreeWidgetItem* FindItemByECharType(CDRMPlot::ECharType eCharType);
	string			ECharTypeToPlotName(CDRMPlot::ECharType eCharType);
	CDRMPlot::ECharType PlotNameToECharType(const string& PlotName);

	QMenu*			pTreeWidgetContextMenu;
	CDRMPlot::ECharType eCurCharType, eNewCharType;
	int				iPlotStyle;
	vector<CDRMPlot*>	vecpDRMPlots;

public slots:
	void OnTimer();
    void on_drmOptions_TimeLinear();
    void on_drmOptions_TimeWiener();
    void on_drmOptions_FrequencyLinear();
    void on_drmOptions_FrequencyDft();
    void on_drmOptions_FrequencyWiener();
    void on_drmOptions_TiSyncFirstPeak();
    void on_drmOptions_TiSyncEnergy();
    void on_drmOptions_noOfIterationsChanged(int);
    void on_drmOptions_ModiMetric(int);
    void on_drmOptions_FlipSpectrum(int);
    void on_drmOptions_RecFilter(int);
    void on_CheckBoxMuteAudio_stateChanged(int);
    void on_CheckBoxReverb_stateChanged(int);
    void on_CheckBoxSaveAudioWave_stateChanged(int);
    void on_CheckBoxWriteLog_stateChanged(int);
    void on_chartSelector_currentItemChanged(QTreeWidgetItem*);
    void on_chartSelector_customContextMenuRequested(const QPoint&);
    void OnTreeWidgetContMenu(bool);
	void UpdatePlotStyle(int);
    void setLogging(bool);

signals:
	void startLogging();
	void stopLogging();
};

#endif

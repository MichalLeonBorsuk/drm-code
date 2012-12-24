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

#ifndef __SYSEVALDLG_H
#define __SYSEVALDLG_H

#include <qtimer.h>
#include <qstring.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <qlineedit.h>
#include <qtooltip.h>
#include <qwt_thermo.h>
#include <qpixmap.h>

#include <qpopupmenu.h>
#include <qlistview.h>
#include "systemevalDlgbase.h"
#include "qt2/DRMPlot.h"
#include "MultColorLED.h"
#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#include "../DrmReceiver.h"
#include "../ReceptLog.h"
#include "../util/Settings.h"

class CGPSReceiver;

/* Definitions ****************************************************************/
/* Define this macro if you prefer the QT-type of displaying date and time */
#define GUI_QT_DATE_TIME_TYPE


/* Classes ********************************************************************/
class systemevalDlg : public systemevalDlgBase
{
	Q_OBJECT

public:
	systemevalDlg(CDRMReceiver&, CSettings&, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, Qt::WFlags f = 0);

	virtual ~systemevalDlg();

	void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
	void StopLogTimers();

protected:
	CDRMReceiver&		DRMReceiver;
	CSettings&		Settings;

	QTimer			Timer;

	virtual void		showEvent(QShowEvent* pEvent);
	virtual void		hideEvent(QHideEvent* pEvent);
	void			UpdateGPS(CParameter&);
	void			UpdateControls();
	void			AddWhatsThisHelp();
	CDRMPlot*		OpenChartWin(CDRMPlot::ECharType eNewType);

	QString			GetRobModeStr();
	QString			GetSpecOccStr();

	QPopupMenu*		pListViewContextMenu;
	vector<CDRMPlot*>	vecpDRMPlots;
	int				iPlotStyle;

	_BOOLEAN		bEdtFrequencyMutex;

public slots:
	void UpdatePlotStyle(int);
	void OnTimer();
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
	void OnCheckWriteLog();
	void OnCheckSaveAudioWAV();
	void OnCheckRecFilter();
	void OnCheckModiMetric();
	void OnListViContMenu();
	void OnListSelChanged(QListViewItem* NewSelIt);
	void OnListRightButClicked(QListViewItem* NewSelIt, const QPoint& iPnt, int iCol);
signals:
        void startLogging();
        void stopLogging();
};
#endif

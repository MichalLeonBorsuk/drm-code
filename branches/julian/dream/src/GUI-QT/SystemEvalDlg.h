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

#ifndef _SYSTEMEVALUATIONDLG_H
#define _SYSTEMEVALUATIONDLG_H

#include "ui_SystemEvalDlg.h"
#include "../ReceiverInterface.h"
#include <QTimer>
#include <vector>

class CSettings;
class CGPSReceiver;
class CDRMPlot;

class SystemEvalDlg : public QDialog, public Ui_SystemEvalDlg
{
	Q_OBJECT

public:
	SystemEvalDlg(ReceiverInterface&, CSettings&, QWidget* parent = 0,
		const char* name = 0, bool modal = false, Qt::WFlags f = 0);

	virtual ~SystemEvalDlg();

	void UpdatePlotStyle() {}

protected:

    virtual void		showEvent(QShowEvent* pEvent);
	virtual void		hideEvent(QHideEvent* pEvent);

    void AddWhatsThisHelp();
    void InitialiseLEDs();
    void UpdateLEDs(CParameter& Parameters);
    void SetStatus(CMultColorLED*, ETypeRxStatus);
    void InitialiseGPS();
    void UpdateGPS(CParameter& Parameters);
    void InitialiseMERetc();
    void UpdateMERetc(CParameter& Parameters);
    void InitialiseFAC();
    void UpdateFAC(CParameter& Parameters);
    void InitialisePlots();
    void showPlots();
    void hidePlots();
    void newPlot(int, const string&);
    void InitialiseFrequency();
    void UpdateFrequency();

    ReceiverInterface&   Receiver;
    CSettings&      Settings;
    CGPSReceiver*   pGPSReceiver;
    CDRMPlot*       plot;
    vector<CDRMPlot*> plots;
    QTimer*         timer;
    QTimer*         timerTuning;
    QTimer*         timerLineEditFrequency;

    bool bTuningInProgress;

public slots:

    void OnTimer();
    void OnTimerTuning();
    void OnTimerLineEditFrequency();
    void OnLineEditFrequencyChanged(const QString&);
    void EnableGPS(bool);
    void OnCheckBoxMuteAudio();
    void OnCheckBoxReverb();
    void OnCheckSaveAudioWav();
    void ShowGPS(bool);
    void OnItemClicked (QTreeWidgetItem*, int);
    void OnCustomContextMenuRequested ( const QPoint&);
    void OnHelpWhatsThis();
};

#endif

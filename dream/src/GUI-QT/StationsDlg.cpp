/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod, Tomi Manninen
 *
 * 5/15/2005 Andrea Russo
 *	- added preview
 * 5/25/2005 Andrea Russo
 *	- added "days" column in stations list view
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

#include "StationsDlg.h"
#include <QMenuBar>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QDateTime>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHeaderView>
#include <iostream>
#include "../util/Hamlib.h"

StationsDlg::StationsDlg(ReceiverInterface& NDRMR, CSettings& NSettings, bool drm,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f) :
	QDialog(parent, f), Ui_StationsDlg(),
	Receiver(NDRMR), Settings(NSettings), Schedule(),
	TimerClock(), TimerSMeter(), TimerEditFrequency(), TimerMonitorFrequency(),
	TimerTuning(),
	bTuningInProgress(false), bReInitOnFrequencyChange(false),
	ScheduleFile(), networkManager(NULL)
{
    setupUi(this);
    /* Set help text for the controls */
    AddWhatsThisHelp();

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(&Schedule);
    proxyModel->setFilterKeyColumn(0); // actually we don't care
    proxyModel->setFilterRole(Qt::UserRole);
    proxyModel->setDynamicSortFilter(true);

    stationsView->setModel(proxyModel);
    stationsView->setSortingEnabled(true);
    stationsView->horizontalHeader()->setVisible(true);

    /* Set up frequency selector control (QWTCounter control) */
    QwtCounterFrequency->setRange(0.0, 30000.0, 1.0);
    QwtCounterFrequency->setNumButtons(3); /* Three buttons on each side */
    QwtCounterFrequency->setIncSteps(QwtCounter::Button1, 1); /* Increment */
    QwtCounterFrequency->setIncSteps(QwtCounter::Button2, 10);
    QwtCounterFrequency->setIncSteps(QwtCounter::Button3, 100);

    /* Set stations preview */
    comboBoxPreview->addItem(tr("Disabled"), 0);
    comboBoxPreview->addItem(tr("5 minutes"), NUM_SECONDS_PREV_5MIN);
    comboBoxPreview->addItem(tr("15 minutes"), NUM_SECONDS_PREV_15MIN);
    comboBoxPreview->addItem(tr("30 minutes"), NUM_SECONDS_PREV_30MIN);

    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(OnUrlFinished(QNetworkReply*)));

    /* Connections ---------------------------------------------------------- */

    if(drm)
    {
	    ScheduleFile = DRMSCHEDULE_INI_FILE_NAME;
	    connect(pushButtonGetUpdate, SIGNAL(clicked(bool)), this, SLOT(OnGetDRMUpdate(bool)));
    }
    else
    {
	    ScheduleFile = AMSCHEDULE_INI_FILE_NAME;
	    connect(pushButtonGetUpdate, SIGNAL(clicked(bool)), this, SLOT(OnGetAnalogUpdate(bool)));
    }

    connect(comboBoxPreview, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSelectPreview(int)));
    connect(checkBoxShowActive, SIGNAL(stateChanged(int)), this, SLOT(OnShowActive(int)));

    connect(&TimerClock, SIGNAL(timeout()), this, SLOT(OnTimerClock()));
    //connect(&TimerSMeter, SIGNAL(timeout()), this, SLOT(OnTimerSMeter()));
    connect(&TimerEditFrequency, SIGNAL(timeout()), this, SLOT(OnTimerEditFrequency()));
    connect(&TimerMonitorFrequency, SIGNAL(timeout()), this, SLOT(OnTimerMonitorFrequency()));
    connect(&TimerTuning, SIGNAL(timeout()), this, SLOT(OnTimerTuning()));
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));

    TimerClock.stop();
    TimerSMeter.stop();
    TimerEditFrequency.stop();
    TimerMonitorFrequency.stop();
    TimerTuning.stop();

    connect(stationsView, SIGNAL(clicked(const QModelIndex&)),
	    this, SLOT(OnItemClicked(const QModelIndex&)));

    connect(QwtCounterFrequency, SIGNAL(valueChanged(double)),
	    this, SLOT(OnFreqCntNewValue(double)));

    connect(ComboBoxFilterTarget, SIGNAL(activated(const QString&)),
	    this, SLOT(OnFilterChanged(const QString&)));
    connect(ComboBoxFilterCountry, SIGNAL(activated(const QString&)),
	    this, SLOT(OnFilterChanged(const QString&)));
    connect(ComboBoxFilterLanguage, SIGNAL(activated(const QString&)),
	    this, SLOT(OnFilterChanged(const QString&)));
}

StationsDlg::~StationsDlg()
{
}
void StationsDlg::showEvent(QShowEvent*)
{
    /* recover window size and position */
    CWinGeom s;
    Settings.Get("GUI Stations", s);
    const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
    if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
	setGeometry(WinGeom);

    QwtCounterFrequency->setValue(Receiver.GetFrequency());

    /* Retrieve the setting saved into the .ini file */
    int seconds = Settings.Get("GUI Stations", "preview", NUM_SECONDS_PREV_5MIN);
    int index = comboBoxPreview->findData(seconds);
    if(index == -1)
    {
	comboBoxPreview->setCurrentIndex(0);
	Schedule.SetSecondsPreview(0);
    }
    else
    {
	comboBoxPreview->setCurrentIndex(index);
	Schedule.SetSecondsPreview(seconds);
    }

    ComboBoxFilterTarget->addItems(Schedule.ListTargets);
    ComboBoxFilterCountry->addItems(Schedule.ListCountries);
    ComboBoxFilterLanguage->addItems(Schedule.ListLanguages);

    QFileInfo fi = QFileInfo(ScheduleFile);
    if (fi.exists()) /* make sure the schedule file exists */
    {
	QString s = tr(" (last update: ") + fi.lastModified().date().toString() + ")";
	pushButtonGetUpdate->setToolTip(s);
    }
    else
    {
	QMessageBox::information(this, "Dream", QString(tr(
	"The file %1 could not be found or contains no data.\n"
	"No stations can be displayed.\n"
	"Try to download this file by using the 'Update' button.")).arg(ScheduleFile));
    }

    /* Update window */
    OnFilterChanged("");

    /* Activate timers when window is shown */
    TimerClock.start(GUI_TIMER); /* Stations list */
    TimerSMeter.start(GUI_TIMER_S_METER);
    TimerMonitorFrequency.start(GUI_TIMER_UPDATE_FREQUENCY);
}

void StationsDlg::hideEvent(QHideEvent*)
{
    /* Deactivate real-time timers */
    TimerClock.stop();
    TimerSMeter.stop();

    /* Set window geometry data in DRMReceiver module */
    QRect WinGeom = geometry();

    CWinGeom c;
    c.iXPos = WinGeom.x();
    c.iYPos = WinGeom.y();
    c.iHSize = WinGeom.height();
    c.iWSize = WinGeom.width();
    Settings.Put("GUI Stations", c);

    /* Store preview settings */
    Settings.Put("GUI Stations", "preview", Schedule.GetSecondsPreview());
#if 0 //TODO get from view
    /* Store sort settings */
    switch (eModulation)
    {
    case DRM:
	    Settings.Put("GUI Stations", "sortcolumndrm", iCurrentSortColumn);
	    Settings.Put("GUI Stations", "sortascendingdrm", bCurrentSortAscending);
	    break;

    case NONE: // not really likely
    break;

    default:
	    Settings.Put("GUI Stations", "sortcolumnanalog", iCurrentSortColumn);
	    Settings.Put("GUI Stations", "sortascendinganalog", bCurrentSortAscending);
	    break;
    }
#endif
}

void StationsDlg::OnFilterChanged(const QString&)
{
	QString target = ComboBoxFilterTarget->currentText();
	if(target=="") target = "[^#]*";
	QString country = ComboBoxFilterCountry->currentText();
	if(country=="") country = "[^#]*";
	QString language = ComboBoxFilterLanguage->currentText();
	if(language=="") language = "[^#]*";
	bool bShowActive = checkBoxShowActive->isChecked();
	QString r = QString("%1#%2#%3#%4").arg(target).arg(country).arg(language).arg(bShowActive?"1":".");
	proxyModel->setFilterRegExp(QRegExp(r));
}

void StationsDlg::OnTimerClock()
{
	/* Get current UTC time */
	time_t ltime;
	time(&ltime);
	struct tm* gmtCur = gmtime(&ltime);

	/* Generate time in format "12:00 UTC" */
	QString strUTCTime = QString().sprintf("%02d:%02d UTC",
		gmtCur->tm_hour, gmtCur->tm_min);

	/* Load the schedule if necessary
	 * Defer if we need to display the clock
	 */
	if ((Schedule.rowCount() == 0) && (TextLabelUTCTime->text() == strUTCTime))
	{
	setCursor(Qt::WaitCursor);
	Schedule.load(ScheduleFile.toStdString());
	setCursor(Qt::ArrowCursor);
	}

	if(gmtCur->tm_sec == 0)
	{
	setCursor(Qt::WaitCursor);
		Schedule.update();
		proxyModel->invalidate();
	setCursor(Qt::ArrowCursor);
	}

	TextLabelUTCTime->setText(strUTCTime);
}

void StationsDlg::OnShowActive(int state)
{
	OnFilterChanged("");
}

void StationsDlg::OnSelectPreview(int index)
{
	Schedule.SetSecondsPreview(comboBoxPreview->itemData(index).toInt());
	Schedule.update();
	OnFilterChanged(""); // kind of
}

void StationsDlg::OnGetAnalogUpdate(bool)
{
    if (QMessageBox::question(this, tr("Dream Schedule Update"),
	QString(tr("Dream tries to download the newest schedule\nfrom the internet.\n\n"
	    "The current file %1 will be overwritten.\n"
	    "Do you want to continue?")).arg(ScheduleFile),
	    QMessageBox::Ok|QMessageBox::Cancel) != QMessageBox::Ok)
    {
	return;
    }
	QDate d = QDate::currentDate();
	int wk = d.weekNumber();
	int yr = d.year();
	QString y,w;
	if(wk <= 13)
	{
		w = "b";
		y = QString::number(yr-1);
	}
	else if(wk <= 43)
	{
		w = "a";
		y = QString::number(yr);
	}
	else
	{
		w = "b";
		y = QString::number(yr);
	}
	QString path = QString(AM_SCHEDULE_UPDATE_URL).arg(w, y.right(2));
    /* Try to download the current schedule. */
    QNetworkReply * reply = networkManager->get(QNetworkRequest(QUrl(path)));
    if(reply == NULL)
    {
	return;
    }
}

void StationsDlg::OnGetDRMUpdate(bool)
{
    if (QMessageBox::question(this, tr("Dream Schedule Update"),
	QString(tr("Dream tries to download the newest schedule\nfrom the internet.\n\n"
	    "The current file %1 will be overwritten.\n"
	    "Do you want to continue?")).arg(ScheduleFile),
	    QMessageBox::Ok|QMessageBox::Cancel) != QMessageBox::Ok)
    {
	return;
    }
    /* Try to download the current schedule. */
    QNetworkReply * reply = networkManager->get(QNetworkRequest(QUrl(DRM_SCHEDULE_UPDATE_URL)));
    if(reply == NULL)
    {
	return;
    }
}

void StationsDlg::OnUrlFinished(QNetworkReply* reply)
{
	/* Check that pointer points to valid object */
	if (reply)
	{
		if (reply->error() != QNetworkReply::NoError)
		{
	    QMessageBox::information(this, "Dream", QString(tr(
		"Update failed. The following things may caused the failure:\n"
		"\t- the internet connection was not set up properly\n"
		"\t- the server is currently not available\n"
		"\t- the file %1 could not be written")).arg(ScheduleFile));
			return;
		}

	QMessageBox::information(this, "Dream", tr("Update successful."));

	/* Save downloaded data to schedule file */
	QFile f(ScheduleFile);
	f.open(QIODevice::WriteOnly);
	f.write(reply->readAll());
	f.close();

	/* trigger reading updated ini-file */
	Schedule.clear();

		/* Add last update information */
		QFileInfo fi = QFileInfo(ScheduleFile);
		if (fi.exists()) /* make sure the schedule file exists (we know it does! */
		{
			QString s = tr(" (last update: ") + fi.lastModified().date().toString() + ")";
			pushButtonGetUpdate->setToolTip(s);
		}
	}
}

void StationsDlg::OnFreqCntNewValue(double)
{
	// wait an inter-digit timeout
	TimerEditFrequency.setSingleShot(true);
	TimerEditFrequency.start(GUI_TIMER_INTER_DIGIT);
	bTuningInProgress = true;
}

void StationsDlg::OnTimerEditFrequency()
{
	// commit the frequency if different
	int iTunedFrequency = Receiver.GetFrequency();
	int iDisplayedFreq = (int)QwtCounterFrequency->value();
	if(iTunedFrequency != iDisplayedFreq)
	{
		Receiver.SetFrequency(iDisplayedFreq);
		bTuningInProgress = true;
		TimerEditFrequency.setSingleShot(true);
		TimerTuning.start(GUI_TIME_TO_TUNE);
	}
	QItemSelectionModel* sm = stationsView->selectionModel();
	if(sm->hasSelection())
	{
		QModelIndex selection = sm->selectedRows(2)[0];
		// deselect table item
		if(selection.data().toInt() != iDisplayedFreq)
		{
			sm->clearSelection();
#if 0 // TODO 0 not working - this should select a row matching the tuned frequency
			for(int i=0; i<proxyModel->rowCount(); i++)
			{
				QModelIndex mi = proxyModel->index(i,2);
				if(mi.data().toInt() == iDisplayedFreq)
				{
					sm->select(mi, QItemSelectionModel::Rows);
				}
			}
#endif
		}
	}
}

void StationsDlg::OnTimerTuning()
{
	bTuningInProgress = false;
}

void StationsDlg::OnTimerMonitorFrequency()
{
	/* Update frequency edit control
	 * frequency could be changed by evaluation dialog
	 * or RSCI
	 */
	int iTunedFrequency = Receiver.GetFrequency();
	int iDisplayedFreq = (int)QwtCounterFrequency->value();
	if(iTunedFrequency == iDisplayedFreq)
	{
		bTuningInProgress = false;
	}
	else
	{
		if(bTuningInProgress == false)
			QwtCounterFrequency->setValue(iTunedFrequency);
	}
}

void StationsDlg::OnItemClicked(const QModelIndex& item)
{
	/* Third column (column 2) of stationsView is frequency.
	   Set value in frequency counter control QWT. Setting this parameter
	   will emit a "value changed" signal which sets the new frequency.
	   Therefore, here no call to "SetFrequency()" is needed.*/
	QModelIndex selection = item.sibling(item.row(), 2);
	QwtCounterFrequency->setValue(selection.data().toInt());
}

void StationsDlg::AddWhatsThisHelp()
{
	/* Stations List */
	stationsView->setWhatsThis(
		tr("<b>Stations List:</b> In the stations list "
		"view all DRM stations which are stored in the DRMSchedule.ini file "
		"are shown. It is possible to show only active stations by changing a "
		"setting in the 'view' menu. The color of the cube on the left of a "
		"menu item shows the current status of the DRM transmission. A green "
		"box shows that the transmission takes place right now, a "
		"yellow cube shows that this is a test transmission and with a "
		"red cube it is shown that the transmission is offline, "
		"a pink cube shown that the transmission soon will be offline.<br>"
		"If the stations preview is active an orange box shows the stations "
		"that will be active.<br>"
		"The list can be sorted by clicking on the headline of the "
		"column.<br>By clicking on a menu item, a remote front-end can "
		"be automatically switched to the current frequency and the "
		"Dream software is reset to a new acquisition (to speed up the "
		"synchronization process). Also, the log-file frequency edit "
		"is automatically updated."));

	/* Frequency Counter */
	QwtCounterFrequency->setWhatsThis(
		tr("<b>Frequency Counter:</b> The current frequency "
		"value can be changed by using this counter. The tuning steps are "
		"100 kHz for the  buttons with three arrows, 10 kHz for the "
		"buttons with two arrows and 1 kHz for the buttons having only "
		"one arrow. By keeping the button pressed, the values are "
		"increased / decreased automatically."));

	/* UTC time label */
	TextLabelUTCTime->setWhatsThis(
		tr("<b>UTC Time:</b> Shows the current Coordinated "
		"Universal Time (UTC) which is nearly the same as Greenwich Mean Time "
		"(GMT)."));

	/* S-meter */
	const QString strSMeter =
		tr("<b>Signal-Meter:</b> Shows the signal strength "
		"level in dB relative to S9.<br>Note that not all "
		"front-ends controlled by hamlib support this feature. If the s-meter "
		"is not available, the controls are disabled.");

	TextLabelSMeter->setWhatsThis(strSMeter);
	ProgrSigStrength->setWhatsThis(strSMeter);
}

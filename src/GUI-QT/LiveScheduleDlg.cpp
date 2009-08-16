/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Julian Cable, Andrea Russo
 *
 * Description:
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

#include "LiveScheduleDlg.h"
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <iostream>

/* Implementation *************************************************************/

void
CLiveScheduleItem::updateState(const time_t ltime, int iPreview)
{
	state = IS_INACTIVE;
	if(schedule.IsActive(ltime) && !schedule.IsActive(ltime+NUM_SECONDS_SOON_INACTIVE))
	{
		state = IS_SOON_INACTIVE;
		return;
	}
	if(schedule.IsActive(ltime))
	{
		state = IS_ACTIVE;
		return;
	}
	if(schedule.IsActive(ltime+iPreview))
	{
		state = IS_PREVIEW;
	}
}

QString
CLiveScheduleItem::ExtractTime() const
{
	int iTimeStart = schedule.iStartTime;
	int iDuration = schedule.iDuration;
	QString sDays = "";
	QString sResult = "";

	if ((iTimeStart == 0) && (iDuration == 0))
		return sResult;

	/* Start time */
	int iStartMinutes = iTimeStart % 60;
	int iStartHours = iTimeStart / 60;

	/* Stop time */
	bool bAllWeek24Hours = false;
	const int iTimeStop = iTimeStart + iDuration;

	int iStopMinutes = iTimeStop % 60;
	int iStopHours = iTimeStop / 60;

	if (iStopHours > 24)
	{
		int iDays = iStopHours / 24;

		if (iDays == 7)
			/* All the week */
			bAllWeek24Hours = true;
		else
		{
			/* Add information about days duration */
			if (iDays > 1)
				sDays.sprintf(" (%d days)", iDays);
			iStopHours = iStopHours % 24;
		}
	}

	if (bAllWeek24Hours == true)
		sResult = "24 hours, 7 days a week";
	else
	{
		sResult.sprintf("%02d:%02d-%02d:%02d", iStartHours, iStartMinutes, iStopHours, iStopMinutes);
		sResult += sDays;
	}

	return sResult;
}

QString
CLiveScheduleItem::ExtractDaysFlagString() const
{
	int iDayCode = schedule.iDayCode;
	string strDaysFlags = "0000000";
	for (int i = 0; i < 7; i++)
	{
		if ((1 << (6 - i)) & iDayCode)
			strDaysFlags[i]++;
	}

	/* Init days string vector */
	const QString strDayDef[] = {
		QObject::tr("Mon"),
		QObject::tr("Tue"),
		QObject::tr("Wed"),
		QObject::tr("Thu"),
		QObject::tr("Fri"),
		QObject::tr("Sat"),
		QObject::tr("Sun")
	};

	QString strDaysShow = "";
	/* First test for day constellations which allow to apply special names */
	/* 1111111 = Mon Tue Wed Thu Fri Sat Sun = 1234567 (1 = Monday, 7 = Sunday) */
	if (strDaysFlags == "1111111")
		strDaysShow = QObject::tr("daily");
	else if (strDaysFlags == "1111100")
		strDaysShow = QObject::tr("weekdays");
	else if (strDaysFlags == "0000011")
		strDaysShow = QObject::tr("weekends");
	else if (strDaysFlags == "1111110")
		strDaysShow = QObject::tr("from Mon to Sat");
	else
	{
		/* No special name could be applied, just list all active days */
		for (size_t i = 0; i < 7; i++)
		{
			/* Check if day is active */
			if (strDaysFlags[i] == CHR_ACTIVE_DAY_MARKER)
			{
				/* Set commas in between the days, to not set a comma at
				   the beginning */
				if (strDaysShow != "")
					strDaysShow += ",";

				/* Add current day */
				strDaysShow += strDayDef[i];
			}
		}
	}
	return strDaysShow;
}

CDRMLiveSchedule::CDRMLiveSchedule():QAbstractTableModel(),
	ScheduleTable(),iSecondsPreview(0),
	dReceiverLatitude(0),dReceiverLongitude(0),
	BitmCubeGreen(13,13),BitmCubeGreenLittle(5, 5),
	BitmCubeYellow(13,13),BitmCubeRed(13,13),
	BitmCubeOrange(13,13),BitmCubePink(13,13)
{
	BitmCubeGreen.fill(QColor(0, 255, 0));
	BitmCubeGreenLittle.fill(QColor(0, 255, 0));
	BitmCubeYellow.fill(QColor(255, 255, 0));
	BitmCubeRed.fill(QColor(255, 0, 0));
	BitmCubeOrange.fill(QColor(255, 128, 0));
	BitmCubePink.fill(QColor(255, 128, 128));
}

void
CDRMLiveSchedule::SetReceiverCoordinates(double latitude, double longitude)
{
	dReceiverLatitude = latitude;
	dReceiverLongitude = longitude;
}

int
CDRMLiveSchedule::rowCount ( const QModelIndex& parent) const
{
	return ScheduleTable.size();
}

int
CDRMLiveSchedule::columnCount ( const QModelIndex&) const
{
	return 6;
}

QVariant
CDRMLiveSchedule::data ( const QModelIndex& index, int role) const
{
	const CLiveScheduleItem& item = ScheduleTable[index.row()];
	switch(role)
	{
	case Qt::DecorationRole:
		if(index.column()==0)
		{
			/* Check, if station is currently transmitting. If yes, set
			   special pixmap */
			QIcon icon;
			switch (item.state)
			{
				case CLiveScheduleItem::IS_ACTIVE:
					icon.addPixmap(BitmCubeGreen);
					break;
				case CLiveScheduleItem::IS_PREVIEW:
					icon.addPixmap(BitmCubeOrange);
					break;
				case CLiveScheduleItem::IS_SOON_INACTIVE:
					icon.addPixmap(BitmCubePink);
					break;
				default:
					icon.addPixmap(BitmCubeRed);
			}
			return icon;
		}
		if(index.column()==4)
		{
			/* If receiver coordinates are in the target area add a little green cube */
			if (item.InsideTargetArea)
			{
				QIcon icon;
				icon.addPixmap(BitmCubeGreen);
				return icon;
			}
		}
		break;
	case Qt::DisplayRole:
		switch(index.column())
		{
			case 0:
				return QString(item.strFreq.c_str()); /* freq. */
			break;
			case 1:
				return QString(item.strStation.c_str()); /* freq. */
				break;
			case 2:
				return QString(item.strSystem.c_str());
				break;
			case 3: // time
				return item.ExtractTime();
				break;
			case 4: // target
				return QString(item.strTarget.c_str());
				break;
			case 5: // days
				return item.ExtractDaysFlagString();
				break;
		}
		break;
	case Qt::UserRole:
		{
			if(item.state!=CLiveScheduleItem::IS_INACTIVE)
				return "1";
			return "0";
		}
		break;
	case Qt::TextAlignmentRole:
		switch(index.column())
		{
			case 0:
				return QVariant(Qt::AlignRight|Qt::AlignVCenter);
			default:
				return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
		}
	}
	return QVariant();
}

QVariant
CDRMLiveSchedule::headerData ( int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole)
		return QVariant();
	if(orientation != Qt::Horizontal)
		return QVariant();
	switch(section)
	{
		case 0: return tr("Frequency [kHz/MHz]"); break;
		case 1: return tr("Station Name/Id"); break;
		case 2: return tr("System"); break;
		case 3: return tr("Time [UTC]"); break;
		case 4: return tr("Target"); break;
		case 5: return tr("Start day"); break;
	}
	return "";
}

void
CDRMLiveSchedule::DecodeTargets(const vector < CAltFreqRegion >
								vecRegions, string & strRegions,
								bool & bIntoTargetArea)
{
	int iCIRAF;
	int iReceiverLatitude = int (dReceiverLatitude);
	int iReceiverLongitude = int (dReceiverLongitude);
	stringstream ssRegions;

	bIntoTargetArea = false;

	for(size_t i = 0; i < vecRegions.size(); i++)
	{
		const int iLatitude = vecRegions[i].iLatitude;
		const int iLongitude = vecRegions[i].iLongitude;

		const int iLatitudeEx = vecRegions[i].iLatitudeEx;
		const int iLongitudeEx = vecRegions[i].iLongitudeEx;

		size_t iCIRAFSize = vecRegions[i].veciCIRAFZones.size();

		if (iCIRAFSize > 0)
		{
			/* Targets */
			for (size_t j = 0; j < iCIRAFSize; j++)
			{
				iCIRAF = vecRegions[i].veciCIRAFZones[j];

				if (ssRegions.str() != "")
					ssRegions << ", ";

				ssRegions << strTableCIRAFzones[iCIRAF];
			}
		}
		else
		{
			/* if ciraf zones aren't defined show the latitude and
			 * longitude of the centre of the target area */

			if (ssRegions.str() != "")
				ssRegions << ", ";

			int iLatitudeMed = (iLatitude + (iLatitudeEx / 2));

			ssRegions << "latitude " << abs(iLatitudeMed) << "\xb0 ";

			if (iLatitudeMed < 0)
				ssRegions << 'S';
			else
				ssRegions << 'N';

			int iLongitudeMed = (iLongitude + (iLongitudeEx / 2));

			if (iLongitudeMed >= 180)
				iLongitudeMed = iLongitudeMed - 360;

			ssRegions << " longitude " << abs(iLongitudeMed) << "\xb0 ";

			if (iLongitudeMed < 0)
				ssRegions << 'W';
			else
				ssRegions << 'E';

		}
		/* check if receiver coordinates are inside target area
		 * TODO check if inside CIRAF zones */
		bool bLongitudeOK = ((iReceiverLongitude >= iLongitude)
										 && (iReceiverLongitude <=
											 (iLongitude + iLongitudeEx)))
					|| (((iLongitude + iLongitudeEx) >= 180)
						&& (iReceiverLongitude <=
							(iLongitude + iLongitudeEx - 360)));

		bool bLatitudeOK = ((iReceiverLatitude >= iLatitude)
										&& (iReceiverLatitude <=
											(iLatitude + iLatitudeEx)));

		bIntoTargetArea = bIntoTargetArea || (bLongitudeOK && bLatitudeOK);
	}
	strRegions = ssRegions.str();
}

void
CDRMLiveSchedule::LoadServiceDefinition(const CServiceDefinition& service,
		const CServiceInformation& ServiceInformation)
{
	string strRegions = "";
	bool bIntoTargetArea = false;
	const CAltFreqSign AltFreqSign = ServiceInformation.AltFreqSign;

	/* Region */
	if (service.iRegionID != 0)
		DecodeTargets(AltFreqSign.vecRegions[service.iRegionID], strRegions, bIntoTargetArea);

	/* For all frequencies */
	for (size_t j = 0; j < service.veciFrequencies.size(); j++)
	{
		if (service.iScheduleID > 0)
		{
			const vector<CAltFreqSched>& vecSchedules = AltFreqSign.vecSchedules[service.iScheduleID];
			for (size_t k = 0; k < vecSchedules.size(); k++)
			{
				CLiveScheduleItem item;

				/* Frequency */
				item.strFreq = service.Frequency(j);

				/* Add the target */
				item.strTarget = strRegions;

				/* Add the schedule */
				item.schedule = vecSchedules[k];

				/* Local receiver coordinates are into target area or not */
				item.InsideTargetArea = bIntoTargetArea;

				/* Add the system (transmission mode) */
				item.strSystem = service.System();

				/* Add the Service ID - 0 for DRM Muxes, ID of the Other Service if present */
				item.iServiceID = ServiceInformation.id;

				if(ServiceInformation.label.size() == 0)
				{
					item.strStation = QString("(%1)").arg(ServiceInformation.id, 0, 16).toStdString();
				}
				else
				{
					item.strStation = *ServiceInformation.label.begin();
				}

				/* Add new item in table */
				ScheduleTable.push_back(item);
			}
		}
		else
		{
			CLiveScheduleItem item;

			/* Frequency */
			item.strFreq = service.Frequency(j);

			/* Add the target */
			item.strTarget = strRegions;

			/* Local receiver coordinates are into target area or not */
			item.InsideTargetArea = bIntoTargetArea;

			/* Add the system (transmission mode) */
			item.strSystem = service.System();

			/* Add the Service ID - 0 for DRM Muxes, ID of the Other Service if present */
			item.iServiceID = ServiceInformation.id;

			/* Add new item in table */
			ScheduleTable.push_back(item);
		}
	}
}

void
CDRMLiveSchedule::LoadAFSInformation(
	const map<uint32_t,CServiceInformation>& ServiceInformation)
{
	size_t i;

	/* Init table for stations */
	ScheduleTable.clear();
	for(map<uint32_t,CServiceInformation>::const_iterator
		item = ServiceInformation.begin(); item != ServiceInformation.end(); item++)
	{
		uint32_t iServiceId = item->first;
		CAltFreqSign AltFreqSign = item->second.AltFreqSign;
		/* Add AFS information for DRM multiplexes */
		for (i = 0; i < AltFreqSign.vecMultiplexes.size(); i++)
		{
			/* TODO multiplex and restrictions */
			//service.bIsSyncMultplx;

			//for ( k = 0; k < 4; k++)
			//  service.veciServRestrict[k];

			LoadServiceDefinition(AltFreqSign.vecMultiplexes[i], item->second);
		}

		/* Add AFS information for Other Services */
		for (i = 0; i < AltFreqSign.vecOtherServices.size(); i++)
		{
			/* TODO same service */
			//OtherService.bSameService;

			LoadServiceDefinition(AltFreqSign.vecOtherServices[i], item->second);
		}
	}
	update();
	reset();
}

void
CDRMLiveSchedule::update()
{
	for(size_t i=0; i<ScheduleTable.size(); i++)
	{
		CLiveScheduleItem::State oldstate = ScheduleTable[i].state;
		ScheduleTable[i].updateState(QDateTime::currentDateTime().toTime_t(), iSecondsPreview);
		if(oldstate != ScheduleTable[i].state)
		{
			emit dataChanged(index(i,0),index(i,0));
		}
	}
}

QString
CDRMLiveSchedule::toHTML(const QString& strStationName)
{
	/* Save to file current schedule  */
	QString strTitle(tr("AFS Live Schedule"));

	/* Prepare HTML page for storing the content */
	QString strText = "<html>\n<head>\n";
	strText += "<meta http-equiv=\"content-Type\" ";
	strText += "content=\"text/html; charset=utf-8\">\n<title>";
	strText += strStationName + " - " + strTitle;
	strText += "</title>\n</head>\n\n<body>\n";
	strText += "<h4>" + strTitle + "</h4>";
	strText += "<h3>" + strStationName + "</h3>";
	strText += "\n<table border=\"1\">";

	for(int i=0; i<columnCount(); i++)
	{
		strText += QString("<th>%1</th>\n").arg(headerData (i, Qt::Horizontal).toString());
	}

	for(int i=0; i<rowCount(); i++)
	{
		for(int j=0; j<columnCount(); j++)
		{
			strText += QString("<td>%1</td>\n").arg(index(i, j).data().toString());
		}
	}
	strText += "</table>\n";
	/* Add current date and time */
	strText += "<br><p align=right><font size=-2><i>";
	strText += QDateTime::currentDateTime().toString() + "</i></font></p>";
	strText += "</body>\n</html>";
	return strText;
}

LiveScheduleDlg::LiveScheduleDlg(ReceiverInterface& NDRMR, CSettings& s,
								 QWidget * parent, const char *name,
								 bool modal, Qt::WFlags f):
QDialog(parent, f), Ui_LiveScheduleDlg(),
DRMReceiver(NDRMR), Settings(s),
strCurrentSavePath(".")
{
    setupUi(this);
	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Set stations preview */
	comboBoxPreview->addItem(tr("Disabled"), 0);
	comboBoxPreview->addItem(tr("5 minutes"), NUM_SECONDS_PREV_5MIN);
	comboBoxPreview->addItem(tr("15 minutes"), NUM_SECONDS_PREV_15MIN);
	comboBoxPreview->addItem(tr("30 minutes"), NUM_SECONDS_PREV_30MIN);

	/* Connections ---------------------------------------------------------- */
	connect(&TimerList, SIGNAL(timeout()), this, SLOT(OnTimerList()));
	connect(&TimerUTCLabel, SIGNAL(timeout()), this, SLOT(OnTimerUTCLabel()));

	connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));
	connect(pushButtonSave, SIGNAL(clicked()), this, SLOT(OnSave()));

	connect(comboBoxPreview, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSelectPreview(int)));
	connect(checkBoxShowActive, SIGNAL(stateChanged(int)), this, SLOT(OnShowActive(int)));

	connect(CheckBoxFreeze, SIGNAL(clicked()), this, SLOT(OnCheckFreeze()));

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(&Schedule);
	proxyModel->setFilterKeyColumn(0); // actually we don't care
	proxyModel->setFilterRole(Qt::UserRole);
	proxyModel->setDynamicSortFilter(true);

	tableView->setModel(proxyModel);

	TimerList.stop();
	TimerUTCLabel.stop();
}

LiveScheduleDlg::~LiveScheduleDlg()
{
}

void
LiveScheduleDlg::showEvent(QShowEvent *)
{
	/* recover window size and position */
	CWinGeom g;
	Settings.Get("Live Schedule", g);
	const QRect WinGeom(g.iXPos, g.iYPos, g.iWSize, g.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

	/* Set sorting behaviour of the list */
	iCurrentSortColumn = Settings.Get("Live Schedule", "sortcolumn", 0);
	bCurrentSortAscending = Settings.Get("Live Schedule", "sortascending", true);
	//ListViewStations->setSorting(iCurrentSortColumn, bCurrentSortAscending);
	/* Retrieve the setting saved into the .ini file */
	string str = strCurrentSavePath.toStdString();
	str = Settings.Get("Live Schedule", "storagepath", str);
	strCurrentSavePath = str.c_str();

	/* Set stations in list view which are active right now */
	bShowAll = Settings.Get("Live Schedule", "showall", false);
	if (bShowAll)
	{
		// TODO
	}

	/* Set stations preview */
	int seconds = Settings.Get("Live Schedule", "preview", NUM_SECONDS_PREV_5MIN);
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

	/* Update window */

	OnTimerUTCLabel();
	SetStationsView();

	OnTimerUTCLabel();
	TimerUTCLabel.start(GUI_TIMER_UTC_TIME_LABEL);

	if (!CheckBoxFreeze->isChecked())
	{
		OnTimerList();

		/* Activate real-time timer when window is shown */
		TimerList.start(GUI_TIMER_LIST_VIEW_UPDATE);	/* Stations list */
	}
}

void
LiveScheduleDlg::hideEvent(QHideEvent *)
{
	/* Deactivate real-time timers */
	TimerList.stop();
	TimerUTCLabel.stop();

	/* save window geometry data */
	QRect WinGeom = geometry();
	CWinGeom c;
	c.iXPos = WinGeom.x();
	c.iYPos = WinGeom.y();
	c.iHSize = WinGeom.height();
	c.iWSize = WinGeom.width();
	Settings.Put("Live Schedule", c);

	/* Store preview settings */
	Settings.Put("Live Schedule", "preview", Schedule.GetSecondsPreview());

	/* Store sort settings */
	Settings.Put("Live Schedule", "sortcolumn", iCurrentSortColumn);
	Settings.Put("Live Schedule", "sortascending", bCurrentSortAscending);

	/* Store preview settings */
	Settings.Put("Live Schedule", "showall", bShowAll);

	/* Store save path */
	string str = strCurrentSavePath.toStdString();
	Settings.Put("Live Schedule", "storagepath", str);
}
void
LiveScheduleDlg::OnFilterChanged(const QString&)
{
	proxyModel->setFilterRegExp(QRegExp("."));
}

void
LiveScheduleDlg::OnItemClicked(const QModelIndex& item)
{
	QModelIndex selection = item.sibling(item.row(), 0);
	//QwtCounterFrequency->setValue(selection.data().toInt());
}

void
LiveScheduleDlg::OnShowActive(int state)
{
	if(state==Qt::Unchecked)
		proxyModel->setFilterRegExp(QRegExp("."));
	else
		proxyModel->setFilterRegExp(QRegExp("1"));
}

void
LiveScheduleDlg::OnSelectPreview(int index)
{
	Schedule.SetSecondsPreview(comboBoxPreview->itemData(index).toInt());
	Schedule.update();
	OnFilterChanged(""); // kind of
}

void
LiveScheduleDlg::OnCheckFreeze()
{
	/* if CheckBoxFreeze is checked the schedule is frozen */
	if (CheckBoxFreeze->isChecked())
		TimerList.stop();
	else
	{
		OnTimerList();
		TimerList.start(GUI_TIMER_LIST_VIEW_UPDATE);	/* Stations list */
	}
}

void LiveScheduleDlg::OnTimerUTCLabel()
{
	/* Get current UTC time */
	time_t ltime;
	time(&ltime);
	struct tm *gmtCur = gmtime(&ltime);

	/* Generate time in format "UTC 12:00" */
	QString strUTCTime = QString().sprintf("%02d:%02d UTC",
										   gmtCur->tm_hour, gmtCur->tm_min);

	/* Only apply if time label does not show the correct time */
	if (TextLabelUTCTime->text().compare(strUTCTime))
		TextLabelUTCTime->setText(strUTCTime);
}

void
LiveScheduleDlg::OnTimerList()
{
	CParameter& Parameters = *DRMReceiver.GetParameters();

	Parameters.Lock();
	/* Get current receiver latitude and longitude if defined */
	if (Parameters.GPSData.GetPositionAvailable())
	{
		double latitude, longitude;
		Parameters.GPSData.GetLatLongDegrees(latitude, longitude);
		Schedule.SetReceiverCoordinates(latitude, longitude);
	}
	Parameters.Unlock();

	/* Update schedule and list view */
	LoadSchedule();
}


void
LiveScheduleDlg::LoadSchedule()
{
	/* Lock mutex for modifying the vecpListItems */
	ListItemsMutex.lock();

	/* save the state of the station id column in case we want it later */
	//iWidthColStationID = ListViewStations->columnWidth(iColStationID);

	CParameter& Parameters = *DRMReceiver.GetParameters();
	Parameters.Lock();
	Schedule.LoadAFSInformation(Parameters.ServiceInformation);
	Parameters.Unlock();

	int iNumStations = Schedule.rowCount();

	/* Enable save button if there are any stations */
	pushButtonSave->setEnabled(iNumStations > 0);

	/* Unlock BEFORE calling the stations view update because in this function
	   the mutex is locked, too! */
	ListItemsMutex.unlock();

	/* Update list view */
	SetStationsView();

	QString strTitle = tr("Live Schedule");

	if (iNumStations > 0)
	{
		Parameters.Lock();
		/* Get current service */
		const int iCurSelAudioServ =
			Parameters.GetCurSelAudioService();

		if (Parameters.Service[iCurSelAudioServ].IsActive())
		{
			/* Do UTF-8 to string conversion with the label strings */
			QString strStationName =
				QString().
				fromUtf8(
						 Parameters.
						  Service[iCurSelAudioServ].strLabel.c_str());

			/* add station name on the title of the dialog */
			if (strStationName != "")
				strTitle += " [" + strStationName.trimmed() + "]";
		}
		Parameters.Unlock();
	}

	setWindowTitle(strTitle);
}

void
LiveScheduleDlg::SetStationsView()
{
	tableView->resizeColumnsToContents();
	/* Set lock because of list view items. These items could be changed
	   by another thread */
    CParameter& Parameters = *DRMReceiver.GetParameters();
	Parameters.Lock();
    int sNo = Parameters.GetCurSelAudioService();
    string thisServiceLabel = Parameters.Service[sNo].strLabel;
	Parameters.Unlock();

	ListItemsMutex.lock();

	const int iNumStations = Schedule.rowCount();

	bool bListHastChanged = false;

	bool bHaveOtherServiceIDs = false;


	if(bHaveOtherServiceIDs)
	{
		//ListViewStations->setColumnText(iColStationID, tr("Station Name/Id"));
		//ListViewStations->setColumnWidth(iColStationID, iWidthColStationID);
	}
	else
	{
		//ListViewStations->setColumnText(iColStationID, "");
		//ListViewStations->setColumnWidth(iColStationID, 0);
	}

	/* Sort the list if items have changed */
	if(bListHastChanged)
	{
		//ListViewStations->sort();
	}

	ListItemsMutex.unlock();
}

QString
ColValue(const QString strValue)
{
	if (strValue == "")
		return "&nbsp;";
	else
		return strValue;
}

void
LiveScheduleDlg::OnSave()
{

	CParameter& Parameters = *DRMReceiver.GetParameters();

	Parameters.Lock();

	const int iCurSelAudioServ =
		Parameters.GetCurSelAudioService();
	/* Do UTF-8 to QString (UNICODE) conversion with the station name strings */
	QString strStationName =
		QString().fromUtf8(Parameters.Service[iCurSelAudioServ].strLabel.c_str());

	Parameters.Unlock();

	/* Lock mutex for use the vecpListItems */
	ListItemsMutex.lock();

	QString strText = Schedule.toHTML(strStationName);

	ListItemsMutex.unlock();

	if(strText != "")
	{
		QString strPath = strCurrentSavePath + "/"
				+ strStationName + "_" + "LiveSchedule.html";

		QString strFileName = QFileDialog::getSaveFileName(this, strPath, "*.html");

		if (!strFileName.isNull())
		{

			/* Save as a text stream */
			QFile FileObj(strFileName);

			if (FileObj.open(QIODevice::WriteOnly))
			{
				QTextStream TextStream(&FileObj);
				TextStream << strText;	/* Actual writing */
				FileObj.close();
				/* TODO ini files are latin 1 but the storage path could contain non-latin characters,
				 * either from the station name or the current filesystem via the file dialog
				 */
				strCurrentSavePath = strFileName;
			}
		}
	}
}

void
LiveScheduleDlg::AddWhatsThisHelp()
{
	/* Stations List */
	tableView->setWhatsThis(
					tr("<b>Live Schedule List:</b> In the live schedule list "
					   "it's possible to view AFS (Alternative Frequency Signalling) "
					   "information transmitted with the current DRM or AMSS signal.</b>"
					   "It is possible to limit the view to active stations by changing a "
					   "setting in the 'view' menu.<br>"
					   "The color of the cube on the left of the "
					   "frequency shows the current status of the transmission.<br>"
					   "A green box shows that the transmission takes place right now "
					   "a red cube it is shown that the transmission is offline, "
					   "a pink cube shown that the transmission soon will be offline.<br>"
					   "If the stations preview is active an orange box shows the stations "
					   "that will be active.<br>"
					   "A little green cube on the left of the target column shows that the receiver"
					   " coordinates (latitude and longitude) stored into Dream settings are within"
					   " the target area of this transmission.<br>"
					   "The list can be sorted by clicking on the headline of the column."));

	/* UTC time label */
	TextLabelUTCTime->setWhatsThis(
					tr("<b>UTC Time:</b> Shows the current Coordinated "
					   "Universal Time (UTC) which is also known as Greenwich Mean Time "
					   "(GMT)."));

	/* Check box freeze */
	CheckBoxFreeze->setWhatsThis(
					tr
					("<b>Freeze:</b> If this check box is selected the live schedule is frozen."));
}

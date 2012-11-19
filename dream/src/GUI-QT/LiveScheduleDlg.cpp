/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2005
 *
 * Author(s):
 *	Andrea Russo
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
# include <qfile.h>
# include <qdir.h>
#if QT_VERSION < 0x040000
# include <qpushbutton.h>
# include <qfiledialog.h>
# include <qtextstream.h>
# include <qheader.h>
# include <qwhatsthis.h>
#else
# include <QFileDialog>
# include <QTextStream>
# include <QDateTime>
# include <QHideEvent>
# include <QShowEvent>
# define CHECK_PTR(x) Q_CHECK_PTR(x)
#endif

/* Implementation *************************************************************/

QString
LiveScheduleDlg::ExtractTime(const CAltFreqSched& schedule)
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
    _BOOLEAN bAllWeek24Hours = FALSE;
    const int iTimeStop = iTimeStart + iDuration;

    int iStopMinutes = iTimeStop % 60;
    int iStopHours = iTimeStop / 60;

    if (iStopHours > 24)
    {
        int iDays = iStopHours / 24;

        if (iDays == 7)
            /* All the week */
            bAllWeek24Hours = TRUE;
        else
        {
            /* Add information about days duration */
            if (iDays > 1)
                sDays.sprintf(" (%d days)", iDays);
            iStopHours = iStopHours % 24;
        }
    }

    if (bAllWeek24Hours == TRUE)
        sResult = "24 hours, 7 days a week";
    else
    {
        sResult.sprintf("%02d:%02d-%02d:%02d", iStartHours, iStartMinutes, iStopHours, iStopMinutes);
        sResult += sDays;
    }

    return sResult;
}

QString
LiveScheduleDlg::ExtractDaysFlagString(const int iDayCode)
{
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
        strDaysShow = QObject::tr("from Mon to Fri");
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

void
CDRMLiveSchedule::SetReceiverCoordinates(double latitude, double longitude)
{
    dReceiverLatitude = latitude;
    dReceiverLongitude = longitude;
}

void
CDRMLiveSchedule::DecodeTargets(const vector < CAltFreqRegion >
                                vecRegions, string & strRegions,
                                _BOOLEAN & bIntoTargetArea)
{
    int iCIRAF;
    int iReceiverLatitude = int (dReceiverLatitude);
    int iReceiverLongitude = int (dReceiverLongitude);
    stringstream ssRegions;

    bIntoTargetArea = FALSE;

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
        _BOOLEAN bLongitudeOK = ((iReceiverLongitude >= iLongitude)
                                 && (iReceiverLongitude <=
                                     (iLongitude + iLongitudeEx)))
                                || (((iLongitude + iLongitudeEx) >= 180)
                                    && (iReceiverLongitude <=
                                        (iLongitude + iLongitudeEx - 360)));

        _BOOLEAN bLatitudeOK = ((iReceiverLatitude >= iLatitude)
                                && (iReceiverLatitude <=
                                    (iLatitude + iLatitudeEx)));

        bIntoTargetArea = bIntoTargetArea || (bLongitudeOK && bLatitudeOK);
    }
    strRegions = ssRegions.str();
}

void
CDRMLiveSchedule::LoadServiceDefinition(const CServiceDefinition& service,
                                        const CAltFreqSign& AltFreqSign, const uint32_t iServiceID)
{
    string strRegions = "";
    _BOOLEAN bIntoTargetArea = FALSE;

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
                CLiveScheduleItem LiveScheduleItem;

                /* Frequency */
                LiveScheduleItem.strFreq = service.Frequency(j);

                /* Add the target */
                LiveScheduleItem.strTarget = strRegions;

                /* Add the schedule */
                LiveScheduleItem.Schedule = vecSchedules[k];

                /* Local receiver coordinates are into target area or not */
                LiveScheduleItem.bInsideTargetArea = bIntoTargetArea;

                /* Add the system (transmission mode) */
                LiveScheduleItem.strSystem = service.System();

                /* Add the Service ID - 0 for DRM Muxes, ID of the Other Service if present */
                LiveScheduleItem.iServiceID = iServiceID;

                /* Add new item in table */
                StationsTable.push_back(LiveScheduleItem);
            }
        }
        else
        {
            CLiveScheduleItem LiveScheduleItem;

            /* Frequency */
            LiveScheduleItem.strFreq = service.Frequency(j);

            /* Add the target */
            LiveScheduleItem.strTarget = strRegions;

            /* Local receiver coordinates are into target area or not */
            LiveScheduleItem.bInsideTargetArea = bIntoTargetArea;

            /* Add the system (transmission mode) */
            LiveScheduleItem.strSystem = service.System();

            /* Add the Service ID - 0 for DRM Muxes, ID of the Other Service if present */
            LiveScheduleItem.iServiceID = iServiceID;

            /* Add new item in table */
            StationsTable.push_back(LiveScheduleItem);
        }
    }
}

void
CDRMLiveSchedule::LoadAFSInformations(const CAltFreqSign& AltFreqSign)
{
    size_t i;

    /* Init table for stations */
    StationsTable.clear();

    /* Add AFS information for DRM multiplexes */
    for (i = 0; i < AltFreqSign.vecMultiplexes.size(); i++)
    {
        /* TODO multiplex and restrictions */
        //service.bIsSyncMultplx;

        //for ( k = 0; k < 4; k++)
        //  service.veciServRestrict[k];

        LoadServiceDefinition(AltFreqSign.vecMultiplexes[i], AltFreqSign);
    }

    /* Add AFS information for Other Services */
    for (i = 0; i < AltFreqSign.vecOtherServices.size(); i++)
    {
        COtherService OtherService = AltFreqSign.vecOtherServices[i];

        /* TODO same service */
        //OtherService.bSameService;

        LoadServiceDefinition(OtherService, AltFreqSign, OtherService.iServiceID);
    }
}

LiveScheduleDlg::LiveScheduleDlg(CDRMReceiver & NDRMR,
                                 QWidget * parent, const char *name,
                                 bool modal, Qt::WFlags f):
    CLiveScheduleDlgBase(parent, name, modal, f),
    DRMReceiver(NDRMR),
#if QT_VERSION >= 0x040000
smallGreenCube(":/icons/smallGreenCube.png"),greenCube(":/icons/greenCube.png"),
redCube(":/icons/redCube.png"),orangeCube(":/icons/organgeCube.png"),pinkCube(":/icons/pinkCube.png"),
#endif
    vecpListItems(), iColStationID(1), iWidthColStationID(0)
{
    setupUi(this);
    /* Set help text for the controls */
    AddWhatsThisHelp();

    /* Clear list box for file names and set up columns */
    ListViewStations->clear();



#if QT_VERSION < 0x040000
    /* Define size of the bitmaps */
    const int iXSize = 13;
    const int iYSize = 13;
    BitmCubeGreen.resize(iXSize, iYSize);
    BitmCubeGreenLittle.resize(5, 5);
    BitmCubeYellow.resize(iXSize, iYSize);
    BitmCubeRed.resize(iXSize, iYSize);
    BitmCubeOrange.resize(iXSize, iYSize);
    BitmCubePink.resize(iXSize, iYSize);
	/* Create bitmaps */
    BitmCubeGreen.fill(QColor(0, 255, 0));
    BitmCubeGreenLittle.fill(QColor(0, 255, 0));
    BitmCubeYellow.fill(QColor(255, 255, 0));
    BitmCubeRed.fill(QColor(255, 0, 0));
    BitmCubeOrange.fill(QColor(255, 128, 0));
    BitmCubePink.fill(QColor(255, 128, 128));

	/* We assume that one column is already there */
    ListViewStations->setColumnText(COL_FREQ, tr("Frequency [kHz/MHz]"));
    iColStationID = ListViewStations->addColumn(tr(""));
    iWidthColStationID = this->fontMetrics().width(tr("Station Name/Id"));
    ListViewStations->addColumn(tr("System"));
    ListViewStations->addColumn(tr("Time [UTC]"));
    ListViewStations->addColumn(tr("Target"));
    ListViewStations->addColumn(tr("Start day"));
    /* Set right alignment for numeric columns */
    ListViewStations->setColumnAlignment(COL_FREQ, Qt::AlignRight);

    /* this for add spaces into the column and show all the header caption */
    vecpListItems.resize(1);
    vecpListItems[0] =
        new MyListLiveViewItem(ListViewStations, "00000000000000000");

    connect(ListViewStations->header(), SIGNAL(clicked(int)), this, SLOT(OnHeaderClicked(int)));
#else
    connect(actionSave,  SIGNAL(triggered()), this, SLOT(OnSave()));

    previewMapper = new QSignalMapper(this);
    previewGroup = new QActionGroup(this);
    showMapper = new QSignalMapper(this);
    showGroup = new QActionGroup(this);
    showGroup->addAction(actionShowOnlyActiveStations);
    showMapper->setMapping(actionShowOnlyActiveStations, 0);
    showGroup->addAction(actionShowAllStations);
    showMapper->setMapping(actionShowAllStations, 1);
    connect(actionShowAllStations, SIGNAL(triggered()), showMapper, SLOT(map()));
    connect(actionShowOnlyActiveStations, SIGNAL(triggered()), showMapper, SLOT(map()));
    connect(showMapper, SIGNAL(mapped(int)), this, SLOT(OnShowStationsMenu(int)));
    previewGroup->addAction(actionDisabled);
    previewMapper->setMapping(actionDisabled, 0);
    previewGroup->addAction(action5minutes);
    previewMapper->setMapping(action5minutes, NUM_SECONDS_PREV_5MIN);
    previewGroup->addAction(action15minutes);
    previewMapper->setMapping(action15minutes, NUM_SECONDS_PREV_15MIN);
    previewGroup->addAction(action30minutes);
    previewMapper->setMapping(action30minutes, NUM_SECONDS_PREV_30MIN);
    connect(actionDisabled, SIGNAL(triggered()), previewMapper, SLOT(map()));
    connect(action5minutes, SIGNAL(triggered()), previewMapper, SLOT(map()));
    connect(action15minutes, SIGNAL(triggered()), previewMapper, SLOT(map()));
    connect(action30minutes, SIGNAL(triggered()), previewMapper, SLOT(map()));
    connect(previewMapper, SIGNAL(mapped(int)), this, SLOT(OnShowPreviewMenu(int)));
#endif

    connect(buttonOk,  SIGNAL(clicked()), this, SLOT(close()));
    //connect(actionGetUpdate, SIGNAL(triggered()), this, SLOT(OnGetUpdate()));

    /* Connections ---------------------------------------------------------- */
    connect(&TimerList, SIGNAL(timeout()), this, SLOT(OnTimerList()));
    connect(&TimerUTCLabel, SIGNAL(timeout()), this, SLOT(OnTimerUTCLabel()));

    /* Check boxes */
    connect(CheckBoxFreeze, SIGNAL(clicked()), this, SLOT(OnCheckFreeze()));

    /* Init UTC time shown with a label control */
    OnTimerUTCLabel();
}

LiveScheduleDlg::~LiveScheduleDlg()
{
}

#if QT_VERSION < 0x040000
void
LiveScheduleDlg::setupUi(QWidget*)
{
    /* Set Menu ************************************************************** */
    /* View menu ------------------------------------------------------------ */
    pViewMenu = new QPopupMenu(this);
    CHECK_PTR(pViewMenu);
    showActiveViewMenuItem = pViewMenu->insertItem(tr("Show &only active stations"), this,
                          SLOT(OnShowStationsMenu(int)), 0, 0);
    showAllViewMenuItem = pViewMenu->insertItem(tr("Show &all stations"), this,
                          SLOT(OnShowStationsMenu(int)), 0, 1);

    /* Stations Preview menu ------------------------------------------------ */
    pPreviewMenu = new QPopupMenu(this);
    CHECK_PTR(pPreviewMenu);
    pPreviewMenu->insertItem(tr("&Disabled"), this,
                             SLOT(OnShowPreviewMenu(int)), 0, 0);
    pPreviewMenu->insertItem(tr("&5 minutes"), this,
                             SLOT(OnShowPreviewMenu(int)), 0, 1);
    pPreviewMenu->insertItem(tr("&15 minutes"), this,
                             SLOT(OnShowPreviewMenu(int)), 0, 2);
    pPreviewMenu->insertItem(tr("&30 minutes"), this,
                             SLOT(OnShowPreviewMenu(int)), 0, 3);
    pViewMenu->insertSeparator();
    pViewMenu->insertItem(tr("Stations &preview"), pPreviewMenu);

    SetStationsView();

    /* File menu ------------------------------------------------------------ */
    pFileMenu = new QPopupMenu(this);
    CHECK_PTR(pFileMenu);
    pFileMenu->insertItem(tr("&Save..."), this, SLOT(OnSave()), Qt::CTRL+Qt::Key_S,0);

    /* Main menu bar -------------------------------------------------------- */
    QMenuBar *pMenu = new QMenuBar(this);
    CHECK_PTR(pMenu);
    pMenu->insertItem(tr("&File"), pFileMenu);
    pMenu->insertItem(tr("&View"), pViewMenu);

    pMenu->setSeparator(QMenuBar::InWindowsStyle);

    /* disable save menu */
    pFileMenu->setItemEnabled(0, FALSE);

    /* Now tell the layout about the menu */
    CLiveScheduleDlgBaseLayout->setMenuBar(pMenu);
}
#endif

void
LiveScheduleDlg::LoadSettings(const CSettings& Settings)
{
    /* recover window size and position */
    CWinGeom s;
    Settings.Get("Live Schedule Dialog", s);
    const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
    if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
        setGeometry(WinGeom);

    /* Set sorting behaviour of the list */
    iCurrentSortColumn = Settings.Get("Live Schedule Dialog", "sortcolumn", 0);
    bCurrentSortAscending = Settings.Get("Live Schedule Dialog", "sortascending", TRUE);
#if QT_VERSION < 0x040000
    ListViewStations->setSorting(iCurrentSortColumn, bCurrentSortAscending);
#else
    ListViewStations->sortItems(iCurrentSortColumn, bCurrentSortAscending?Qt::AscendingOrder:Qt::DescendingOrder);
#endif
    /* Retrieve the setting saved into the .ini file */
    strCurrentSavePath = QString::fromUtf8(DRMReceiver.GetParameters()->GetDataDirectory("AFS").c_str());

    /* and make sure it exists */
	CreateDirectories(strCurrentSavePath);

    /* Set stations in list view which are active right now */
    bool bShowAll = Settings.Get("Live Schedule Dialog", "showall", false);
    int iPrevSecs = Settings.Get("Live Schedule Dialog", "preview", 0);

#if QT_VERSION < 0x040000
    if (bShowAll)
        pViewMenu->setItemChecked(showAllViewMenuItem, TRUE);
    else
        pViewMenu->setItemChecked(showActiveViewMenuItem, TRUE);

    /* Set stations preview */
    switch (iPrevSecs)
    {
    case NUM_SECONDS_PREV_5MIN:
        pPreviewMenu->setItemChecked(1, TRUE);
        DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_5MIN);
        break;

    case NUM_SECONDS_PREV_15MIN:
        pPreviewMenu->setItemChecked(2, TRUE);
        DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_15MIN);
        break;

    case NUM_SECONDS_PREV_30MIN:
        pPreviewMenu->setItemChecked(3, TRUE);
        DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_30MIN);
        break;

    default:/* case 0, also takes care of out of value parameters */
        pPreviewMenu->setItemChecked(0, TRUE);
        DRMSchedule.SetSecondsPreview(0);
        break;
    }
#else
    if(bShowAll)
        actionShowAllStations->setChecked(true);
    else
        actionShowOnlyActiveStations->setChecked(true);
    DRMSchedule.SetSecondsPreview(iPrevSecs);
    switch (iPrevSecs)
    {
    case NUM_SECONDS_PREV_5MIN:
        action5minutes->setChecked(true);
        break;

    case NUM_SECONDS_PREV_15MIN:
        action5minutes->setChecked(true);
        break;

    case NUM_SECONDS_PREV_30MIN:
        action30minutes->setChecked(true);
        break;

    default: /* case 0, also takes care of out of value parameters */
        actionDisabled->setChecked(true);
        break;
    }
#endif

}

void
LiveScheduleDlg::SaveSettings(CSettings& Settings)
{
    /* save window geometry data */
    QRect WinGeom = geometry();
    CWinGeom c;
    c.iXPos = WinGeom.x();
    c.iYPos = WinGeom.y();
    c.iHSize = WinGeom.height();
    c.iWSize = WinGeom.width();
    Settings.Put("Live Schedule Dialog", c);

    /* Store preview settings */
    Settings.Put("Live Schedule Dialog", "preview", DRMSchedule.GetSecondsPreview());

    /* Store sort settings */
    Settings.Put("Live Schedule Dialog", "sortcolumn", iCurrentSortColumn);
    Settings.Put("Live Schedule Dialog", "sortascending", bCurrentSortAscending);

    /* Store preview settings */
    Settings.Put("Live Schedule Dialog", "showall", showAll());
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

int LiveScheduleDlg::currentSortColumn()
{
#if QT_VERSION < 0x030000
	return iCurrentSortColumn;
#else
	return ListViewStations->sortColumn();
#endif
}

_BOOLEAN LiveScheduleDlg::showAll()
{
#if QT_VERSION < 0x040000
	return pViewMenu->isItemChecked(showAllViewMenuItem);
#else
	return actionShowAllStations->isChecked();
#endif
}

void
LiveScheduleDlg::OnTimerUTCLabel()
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
LiveScheduleDlg::OnShowStationsMenu(int iID)
{
    /* Update list view */
    SetStationsView();
#if QT_VERSION < 0x040000
    /* Taking care of checks in the menu */
    pViewMenu->setItemChecked(showActiveViewMenuItem, showActiveViewMenuItem == iID);
    pViewMenu->setItemChecked(showAllViewMenuItem, showAllViewMenuItem == iID);
#endif
}

void
LiveScheduleDlg::OnShowPreviewMenu(int iID)
{
#if QT_VERSION < 0x040000
    switch (iID)
    {
    case 1:
        DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_5MIN);
        break;

    case 2:
        DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_15MIN);
        break;

    case 3:
        DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_30MIN);
        break;

    default:					/* case 0: */
        DRMSchedule.SetSecondsPreview(0);
        break;
    }

    /* Update list view */
    SetStationsView();

    /* Taking care of checks in the menu */
    pPreviewMenu->setItemChecked(0, 0 == iID);
    pPreviewMenu->setItemChecked(1, 1 == iID);
    pPreviewMenu->setItemChecked(2, 2 == iID);
    pPreviewMenu->setItemChecked(3, 3 == iID);
#else
    DRMSchedule.SetSecondsPreview(iID);
#endif
}

void
LiveScheduleDlg::OnTimerList()
{
    CParameter& Parameters = *DRMReceiver.GetParameters();

    Parameters.Lock();
    /* Get current receiver latitude and longitude if defined */
    if (Parameters.gps_data.set&LATLON_SET)
    {
        DRMSchedule.SetReceiverCoordinates(
		Parameters.gps_data.fix.latitude, Parameters.gps_data.fix.longitude);
    }
    Parameters.Unlock();

    /* Update schedule and list view */
    LoadSchedule();
}

QString
MyListLiveViewItem::key(int column, bool ascending) const
{
    /* Reimplement "key()" function to get correct sorting behaviour */

    const float fFreq = text(column).toFloat();

    if (column == COL_FREQ)
    {
        /* These columns are filled with numbers. Some items may have numbers
           after the decimal, therefore multiply with 10000 (which moves the
           numbers in front of the comma). Afterwards append zeros at the
           beginning so that positive integer numbers are sorted correctly */
#if QT_VERSION < 0x040000
        return QString().setNum(long(fFreq * 10000.0)).rightJustify(20, '0');
#else
        return QString().setNum(long(fFreq * 10000.0)).rightJustified(20, '0');
#endif
    }
    else
    {
        /* is a text column */
        /* sort the column and then sort for frequency */
        float d = 0.0;

        if (!ascending)
            d = 100000.0;

#if QT_VERSION < 0x040000
        const QString sFreq = QString().setNum(long((fFreq - d) * 10000.0)).rightJustify(20, '0');
        return text(column).lower() + "|" + sFreq;
#else
		const QString sFreq = QString().setNum(long((fFreq - d) * 10000.0)).rightJustified(20, '0');
        return text(column).toLower() + "|" + sFreq;
#endif
    }
}

void
LiveScheduleDlg::LoadSchedule()
{
    /* Lock mutex for modifying the vecpListItems */
    ListItemsMutex.lock();

    /* save the state of the station id column in case we want it later */
    iWidthColStationID = ListViewStations->columnWidth(iColStationID);

    /* Delete all old list view items (it is important that the vector
       "vecpListItems" was initialized to 0 at creation of the global object
       otherwise this may cause an segmentation fault) */
    for (size_t i = 0; i < vecpListItems.size(); i++)
    {
        if (vecpListItems[i] != NULL)
            delete vecpListItems[i];
    }
    vecpListItems.clear();

    CParameter& Parameters = *DRMReceiver.GetParameters();
    Parameters.Lock();
    DRMSchedule.LoadAFSInformations(Parameters.AltFreqSign);
    Parameters.Unlock();

    /* Init vector for storing the pointer to the list view items */
    const int iNumStations = DRMSchedule.GetStationNumber();

    vecpListItems.resize(iNumStations, NULL);

#if QT_VERSION < 0x040000
    /* Enable disable save menu item */
    pFileMenu->setItemEnabled(0, (iNumStations > 0));
#else
    actionSave->setEnabled(iNumStations > 0);
#endif
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
            QString strStationName = QString().fromUtf8(
#if QT_VERSION < 0x040000
				QCString(Parameters.Service[iCurSelAudioServ].strLabel.c_str())
#else
				Parameters.Service[iCurSelAudioServ].strLabel.c_str()
#endif
				);

            /* add station name on the title of the dialog */
            if (strStationName != "")
#if QT_VERSION < 0x040000
                strTitle += " [" + strStationName.stripWhiteSpace() + "]";
#else
                strTitle += " [" + strStationName.trimmed() + "]";
#endif
        }
        Parameters.Unlock();
    }

#if QT_VERSION < 0x040000
	SetDialogCaption(this, strTitle);
#else
	setWindowTitle(strTitle);
#endif
}

void
LiveScheduleDlg::showEvent(QShowEvent* e)
{
	EVENT_FILTER(e);

    /* Update window */
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
LiveScheduleDlg::hideEvent(QHideEvent* e)
{
	EVENT_FILTER(e);

    /* Deactivate real-time timers */
    TimerList.stop();
    TimerUTCLabel.stop();

}

void
LiveScheduleDlg::SetStationsView()
{
    /* Set lock because of list view items. These items could be changed
       by another thread */
    CParameter& Parameters = *DRMReceiver.GetParameters();
    Parameters.Lock();
    int sNo = Parameters.GetCurSelAudioService();
    string thisServiceLabel = Parameters.Service[sNo].strLabel;
    Parameters.Unlock();

    ListItemsMutex.lock();

    const int iNumStations = DRMSchedule.GetStationNumber();

    _BOOLEAN bListHastChanged = FALSE;

    _BOOLEAN bHaveOtherServiceIDs = FALSE;

    /* Add new item for each station in list view */
    for (int i = 0; i < iNumStations; i++)
    {
        if (!((showAll() == FALSE) &&
                (DRMSchedule.CheckState(i) == CDRMLiveSchedule::IS_INACTIVE)))
        {
            /* Only insert item if it is not already in the list */
            if (vecpListItems[i] == NULL)
            {
                /* Generate new list item with all necessary column entries */
                const CLiveScheduleItem& item = DRMSchedule.GetItem(i);
                QString name = "";

                if(item.iServiceID != SERV_ID_NOT_USED)
                {
                    bHaveOtherServiceIDs = TRUE;

                    Parameters.Lock();
                    map <uint32_t,CServiceInformation>::const_iterator
                    si = Parameters.ServiceInformation.find(item.iServiceID);
                    if(si != Parameters.ServiceInformation.end())
                        name = QString::fromUtf8(si->second.label.begin()->c_str());
                    else
                    {
                        ulong sid = item.iServiceID;
                        name = QString("(%1)").arg(sid, 0, 16);
                    }
                    Parameters.Unlock();
                }

                vecpListItems[i] = new MyListLiveViewItem(ListViewStations,
                        QString(item.strFreq.c_str()) /* freq. */ ,
                        name /* station name or id or blank */ ,
                        QString(item.strSystem.c_str()) /* system */ ,
                        ExtractTime(item.Schedule) /* time */,
                        QString(item.strTarget.c_str()) /* target */ ,
                        ExtractDaysFlagString(item.Schedule.iDayCode) /* Show list of days */
                                                         );

                /* Set flag for sorting the list */
                bListHastChanged = TRUE;
            }

#if QT_VERSION < 0x040000
            /* If receiver coordinates are within the target area add a little green cube */
            if (DRMSchedule.GetItem(i).bInsideTargetArea == TRUE)
                vecpListItems[i]->setPixmap(COL_TARGET, BitmCubeGreenLittle);

            /* Check, if station is currently transmitting. If yes, set
               special pixmap */
            switch (DRMSchedule.CheckState(i))
            {
            case CDRMLiveSchedule::IS_ACTIVE:
                vecpListItems[i]->setPixmap(COL_FREQ, BitmCubeGreen);
                break;
            case CDRMLiveSchedule::IS_PREVIEW:
                vecpListItems[i]->setPixmap(COL_FREQ, BitmCubeOrange);
                break;
            case CDRMLiveSchedule::IS_SOON_INACTIVE:
                vecpListItems[i]->setPixmap(COL_FREQ, BitmCubePink);
                break;
            case CDRMLiveSchedule::IS_INACTIVE:
                vecpListItems[i]->setPixmap(COL_FREQ, BitmCubeRed);
                break;
            default:
                vecpListItems[i]->setPixmap(COL_FREQ, BitmCubeRed);
                break;
            }
#else
            /* If receiver coordinates are within the target area add a little green cube */
            if (DRMSchedule.GetItem(i).bInsideTargetArea == TRUE)
                vecpListItems[i]->setIcon(COL_TARGET, smallGreenCube);

            /* Check, if station is currently transmitting. If yes, set
               special pixmap */
            switch (DRMSchedule.CheckState(i))
            {
            case CDRMLiveSchedule::IS_ACTIVE:
                vecpListItems[i]->setIcon(COL_FREQ, greenCube);
                break;
            case CDRMLiveSchedule::IS_PREVIEW:
                vecpListItems[i]->setIcon(COL_FREQ, orangeCube);
                break;
            case CDRMLiveSchedule::IS_SOON_INACTIVE:
                vecpListItems[i]->setIcon(COL_FREQ, pinkCube);
                break;
            case CDRMLiveSchedule::IS_INACTIVE:
                vecpListItems[i]->setIcon(COL_FREQ, redCube);
                break;
            default:
                vecpListItems[i]->setIcon(COL_FREQ, redCube);
                break;
            }
#endif
        }
        else
        {
            /* Delete this item since it is not used anymore */
            if (vecpListItems[i] != NULL)
            {
                /* If one deletes a menu item in QT list view, it is
                   automaticall removed from the list and the list gets
                   repainted */
                delete vecpListItems[i];

                /* Reset pointer so we can distinguish if it is used or not */
                vecpListItems[i] = NULL;

                /* Set flag for sorting the list */
                bListHastChanged = TRUE;
            }
        }
    }


    if(bHaveOtherServiceIDs)
    {
        ListViewStations->setColumnWidth(iColStationID, iWidthColStationID);
    }
    else
    {
        ListViewStations->setColumnWidth(iColStationID, 0);
    }

    /* Sort the list if items have changed */
#if QT_VERSION < 0x040000
    if(bListHastChanged)
        ListViewStations->sort();
#else
    if(bListHastChanged)
        ListViewStations->sortItems(iCurrentSortColumn, bCurrentSortAscending?Qt::AscendingOrder:Qt::DescendingOrder);
#endif
    ListItemsMutex.unlock();
}

void
LiveScheduleDlg::OnHeaderClicked(int c)
{
    /* Store the "direction" of sorting */
    if (iCurrentSortColumn == c)
        bCurrentSortAscending = !bCurrentSortAscending;
    else
        bCurrentSortAscending = TRUE;

    iCurrentSortColumn = c;
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
    QString strFilename;
    QString strSchedule = "";
    QString strValue = "";

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

#if QT_VERSION < 0x040000
    /* Force the sort for all items */
	ListViewStations->sort();

    /* Extract values from the list */

    QListViewItem *myItem = ListViewStations->firstChild();
    while (myItem)
    {
        strSchedule += "<tr>" "<td align=\"right\">" + myItem->text(COL_FREQ) + "</td>"	/* freq */
                       "<td>" + ColValue(myItem->text(1)) + "</td>"	/* system */
                       "<td>" + ColValue(myItem->text(2)) + "</td>"	/* time */
                       "<td>" + ColValue(myItem->text(3)) + "</td>"	/* target */
                       "<td>" + ColValue(myItem->text(4)) + "</td>"	/* days */
                       "</tr>\n";
        myItem = myItem->nextSibling();
    }
#else
    /* Force the sort for all items */
	ListViewStations->sortItems(iCurrentSortColumn, bCurrentSortAscending?Qt::AscendingOrder:Qt::DescendingOrder);

    /* Extract values from the list */

	for(int i=0; i<ListViewStations->topLevelItemCount(); i++)
	{
		QTreeWidgetItem* myItem = ListViewStations->topLevelItem(i);
        strSchedule += "<tr>" "<td align=\"right\">" + myItem->text(COL_FREQ) + "</td>"	/* freq */
                       "<td>" + ColValue(myItem->text(1)) + "</td>"	/* system */
                       "<td>" + ColValue(myItem->text(2)) + "</td>"	/* time */
                       "<td>" + ColValue(myItem->text(3)) + "</td>"	/* target */
                       "<td>" + ColValue(myItem->text(4)) + "</td>"	/* days */
                       "</tr>\n";
	}
#endif

    ListItemsMutex.unlock();

    if (strSchedule != "")
    {
        /* Save to file current schedule  */
        QString strTitle(tr("AFS Live Schedule"));
        QString strItems("");

        /* Prepare HTML page for storing the content */
        QString strText = "<html>\n<head>\n"
                          "<meta http-equiv=\"content-Type\" "
                          "content=\"text/html; charset=utf-8\">\n<title>"
                          + strStationName + " - " + strTitle +
                          "</title>\n</head>\n\n<body>\n"
                          "<h4>" + strTitle + "</h4>"
                          "<h3>" + strStationName + "</h3>"
                          "\n<table border=\"1\"><tr>\n"
                          "<th>" + tr("Frequency [kHz/MHz]") + "</th>"
                          "<th>" + tr("System") + "</th>"
                          "<th>" + tr("Time [UTC]") + "</th>"
                          "<th>" + tr("Target") + "</th>"
                          "<th>" + tr("Start day") + "</th>\n"
                          "</tr>\n" + strSchedule + "</table>\n"
                          /* Add current date and time */
                          "<br><p align=right><font size=-2><i>" +
                          QDateTime().currentDateTime().toString() + "</i></font></p>"
                          "</body>\n</html>";

        QString strPath = strCurrentSavePath +
                          strStationName + "_" + "LiveSchedule.html";
#if QT_VERSION < 0x040000
        strFilename = QFileDialog::getSaveFileName(strPath, "*.html", this);
#else
        strFilename = QFileDialog::getSaveFileName(this, "*.html", strPath);
#endif

        if (!strFilename.isEmpty())
        {
            /* Save as a text stream */
            QFile FileObj(strFilename);

#if QT_VERSION < 0x040000
            if (FileObj.open(IO_WriteOnly))
#else
            if (FileObj.open(QIODevice::WriteOnly))
#endif
            {
                QTextStream TextStream(&FileObj);
                TextStream << strText;	/* Actual writing */
                FileObj.close();
#if QT_VERSION < 0x040000
                strCurrentSavePath = QFileInfo(strFilename).filePath() + "/";
#else
                strCurrentSavePath = QFileInfo(strFilename).path() + "/";
#endif
            }
        }
    }
}

void
LiveScheduleDlg::AddWhatsThisHelp()
{
    /* Stations List */
	QString strView =
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
                        "The list can be sorted by clicking on the headline of the column.");

    /* UTC time label */
	QString strTime =
                     tr("<b>UTC Time:</b> Shows the current Coordinated "
                        "Universal Time (UTC) which is also known as Greenwich Mean Time "
                        "(GMT).");

    /* Check box freeze */
	QString strFreeze = tr("<b>Freeze:</b> If this check box is selected the live schedule is frozen.");
#if QT_VERSION < 0x040000
    QWhatsThis::add(ListViewStations, strView);
    QWhatsThis::add(TextLabelUTCTime, strTime);
    QWhatsThis::add(CheckBoxFreeze, strFreeze);
#else
	ListViewStations->setWhatsThis(strView);
    TextLabelUTCTime->setWhatsThis(strTime);
    CheckBoxFreeze->setWhatsThis(strFreeze);
#endif
}

CDRMLiveSchedule::StationState CDRMLiveSchedule::CheckState(const int iPos)
{
    /* Get system time */
    time_t
    ltime;
    time(&ltime);

    if (IsActive(iPos, ltime) == TRUE)
    {
        /* Check if the station soon will be inactive */
        if (IsActive(iPos, ltime + NUM_SECONDS_SOON_INACTIVE) == TRUE)
            return IS_ACTIVE;
        else
            return IS_SOON_INACTIVE;
    }
    else
    {
        /* Station is not active, check preview condition */
        if (iSecondsPreview > 0)
        {
            if (IsActive(iPos, ltime + iSecondsPreview) == TRUE)
                return IS_PREVIEW;
            else
                return IS_INACTIVE;
        }
        else
            return IS_INACTIVE;
    }
}

_BOOLEAN
CDRMLiveSchedule::IsActive(const int iPos, const time_t ltime)
{
    return StationsTable[iPos].IsActive(ltime);
}

_BOOLEAN
CLiveScheduleItem::IsActive(const time_t ltime)
{
    return Schedule.IsActive(ltime);
}

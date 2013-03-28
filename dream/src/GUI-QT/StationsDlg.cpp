/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod, Tomi Manninen, Andrea Russo,
 *      Julian Cable
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
#include "../tables/TableStations.h"
#include "DialogUtil.h"
#ifdef HAVE_LIBHAMLIB
# include "../util-QT/Rig.h"
# include "RigDlg.h"
#endif
#include <QHideEvent>
#include <QShowEvent>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QFileInfo>
#include <cmath>

/* Implementation *************************************************************/
CDRMSchedule::CDRMSchedule():
    ListTargets(), ListCountries(), ListLanguages(),
    StationsTable(),eSchedMode(SM_DRM),iSecondsPreviewdrm(0),iSecondsPreviewanalog(0)
{
    SetAnalogUrl();
}

void CDRMSchedule::SetSecondsPreview(int iSec)
{
    if(eSchedMode==SM_DRM)
	iSecondsPreviewdrm = iSec;
    else
	iSecondsPreviewanalog = iSec;
}

int CDRMSchedule::GetSecondsPreview()
{
    if(eSchedMode==SM_DRM)
	return iSecondsPreviewdrm;
    else
	return iSecondsPreviewanalog;
}

void CDRMSchedule::SetSchedMode(const ESchedMode eNewSchM)
{
    eSchedMode = eNewSchM;
    StationsTable.clear();
    if(eSchedMode==SM_DRM)
    {
        schedFileName = DRMSCHEDULE_INI_FILE_NAME;
    }
    else
    {
        schedFileName = AMSCHEDULE_CSV_FILE_NAME;
    }
}

void CDRMSchedule::UpdateStringListForFilter(const CStationsItem& StationsItem)
{
    if (!ListTargets.contains(StationsItem.strTarget))
        ListTargets.append(StationsItem.strTarget);

    if (!ListCountries.contains(StationsItem.strCountry))
        ListCountries.append(StationsItem.strCountry);

    if (!ListLanguages.contains(StationsItem.strLanguage))
        ListLanguages.append(StationsItem.strLanguage);
}

void CDRMSchedule::LoadSchedule()
{
    string filename(schedFileName.toUtf8().constData());
    QApplication::setOverrideCursor(Qt::BusyCursor);
    ListTargets = QStringList("");
    ListCountries = QStringList("");
    ListLanguages = QStringList("");
    StationsTable.clear();
    FILE* pFile = fopen(filename.c_str(), "r");
    if(pFile)
    {
        if(schedFileName.contains("ini"))
            ReadINIFile(pFile);
        else
            ReadCSVFile(pFile);
        fclose(pFile);
    }
    ListTargets.sort();
    ListCountries.sort();
    ListLanguages.sort();
    QApplication::restoreOverrideCursor();
}

void CDRMSchedule::ReadINIFile(FILE* pFile)
{
    const int	iMaxLenName = 256;
    char	cName[iMaxLenName], *r;
    int		iFileStat, n;
    _BOOLEAN	bReadOK = TRUE;

    r = fgets(cName, iMaxLenName, pFile); /* [DRMSchedule] */
    do
    {
        CStationsItem StationsItem;

        /* Start stop time */
        int iStartTime, iStopTime;
        iFileStat = fscanf(pFile, "StartStopTimeUTC=%04d-%04d\n",
                           &iStartTime, &iStopTime);

        if (iFileStat != 2)
            bReadOK = FALSE;
        else
        {
            StationsItem.SetStartTime(iStartTime);
            StationsItem.SetStopTime(iStopTime);
        }

        /* Days */
        /* Init days with the "irregular" marker in case no valid string could
           be read */
        QString strNewDaysFlags = FLAG_STR_IRREGULAR_TRANSM;

        iFileStat = fscanf(pFile, "Days[SMTWTFS]=%255[^\n|^\r]\n", cName);
        if (iFileStat != 1)
            n = fscanf(pFile, "\n");
        else
        {
            /* Check for length of input string (must be 7) */
            QString strTMP(cName);
            if (strTMP.length() == 7)
                strNewDaysFlags = strTMP;
        }

        /* Frequency */
        iFileStat = fscanf(pFile, "Frequency=%d\n", &StationsItem.iFreq);
        if (iFileStat != 1)
            bReadOK = FALSE;

        /* Target */
        iFileStat = fscanf(pFile, "Target=%255[^\n|^\r]\n", cName);
        if (iFileStat != 1)
            n = fscanf(pFile, "\n");
        else
            StationsItem.strTarget = cName;

        /* Power */
        iFileStat = fscanf(pFile, "Power=%255[^\n|^\r]\n", cName);
        if (iFileStat != 1)
            n = fscanf(pFile, "\n");
        else
            StationsItem.rPower = QString(cName).toFloat();

        /* Name of the station */
        iFileStat = fscanf(pFile, "Programme=%255[^\n|^\r]\n", cName);
        if (iFileStat != 1)
            n = fscanf(pFile, "\n");
        else
            StationsItem.strName = cName;

        /* Language */
        iFileStat = fscanf(pFile, "Language=%255[^\n|^\r]\n", cName);
        if (iFileStat != 1)
            n = fscanf(pFile, "\n");
        else
            StationsItem.strLanguage = cName;

        /* Site */
        iFileStat = fscanf(pFile, "Site=%255[^\n|^\r]\n", cName);
        if (iFileStat != 1)
            n = fscanf(pFile, "\n");
        else
            StationsItem.strSite = cName;

        /* Country */
        iFileStat = fscanf(pFile, "Country=%255[^\n|^\r]\n", cName);
        if (iFileStat != 1)
            n = fscanf(pFile, "\n");
        else
            StationsItem.strCountry = cName;

        iFileStat = fscanf(pFile, "\n");

        /* Check for error before applying data */
        if (bReadOK == TRUE)
        {
            /* Set "days flag string" and generate strings for displaying active
               days */
            StationsItem.SetDaysFlagString(strNewDaysFlags);

            /* Add new item in table */
            StationsTable.push_back(StationsItem);

            UpdateStringListForFilter(StationsItem);
        }
    } while (!((iFileStat == EOF) || (bReadOK == FALSE)));

    (void)r; (void)n;
}

void CDRMSchedule::ReadCSVFile(FILE* pFile)
{
    const int	iMaxLenRow = 1024;
    char		cRow[iMaxLenRow];
    CStationData data;

    do {
        CStationsItem StationsItem;

        char* r = fgets(cRow, iMaxLenRow, pFile); (void)r;
        QStringList fields;
        stringstream ss(cRow);
        do {
            string s;
            getline(ss, s, ';');
            fields.push_back(s.c_str());
        } while(!ss.eof());

        StationsItem.iFreq = floor(fields[0].toFloat());

        if(fields[1] == "")
        {
            StationsItem.SetStartTime(0);
            StationsItem.SetStopTime(2400);
        }
        else
        {
            QStringList times = fields[1].split("-");
			if(times.count()==2)
			{
				StationsItem.SetStartTime(times[0].toInt());
				StationsItem.SetStopTime(times[1].toInt());
			}
        }

        if(fields[2].length()>0)
        {
            stringstream ss(fields[2].toUtf8().constData());
            char c;
            enum Days { Sunday=0, Monday=1, Tuesday=2, Wednesday=3,
                        Thursday=4, Friday=5, Saturday=6
                      };
            Days first=Sunday, last=Sunday;
            enum { no, in, done } range_state = no;
            // Days[SMTWTFS]=1111111
            QString strDays = "0000000";
            while(!ss.eof())
            {
                ss >> c;
                switch(c)
                {
                case '-':
                    range_state = in;
                    break;
                case 'M':
                    ss >> c;
                    last = Monday;
                    break;
                case 'T':
                    ss >> c;
                    last = (c=='u')?Tuesday:Thursday;
                    break;
                case 'W':
                    ss >> c;
                    last = Wednesday;
                    break;
                case 'F':
                    ss >> c;
                    last = Friday;
                    break;
                case 'S':
                    ss >> c;
                    last = (c=='u')?Sunday:Saturday;
                    break;
                }
                switch(range_state)
                {
                case no:
                    strDays[last] = '1';
                    break;
                case in:
                    first = last;
                    range_state = done;
                    break;
                case done:
                    if(first<last)
                    {
                        for(int d=first; d<=last; d++)
                            strDays[d] = '1';
                    }
                    range_state = no;
                    break;
                }
            }
            StationsItem.SetDaysFlagString(strDays);
        }
        else
            StationsItem.SetDaysFlagString("1111111");

        //StationsItem.rPower = 0.0;
//0   ;1        ;2    ;3  ;4               ;5;6;7;8;9;10
//1170;1600-1700;Mo-Fr;USA;Voice of America;E; ; ;0; ;
        string homecountry;
        int fieldcount = fields.size();
        if(fieldcount>3)
        {
            homecountry = fields[3].toUtf8().constData();
            string c = data.itu_r_country(homecountry);
            if(c == "")
                c = homecountry;
            StationsItem.strCountry = QString::fromUtf8(c.c_str());
        }

        if(fieldcount>4)
            StationsItem.strName = fields[4];

        if(fieldcount>5)
        {
            string l = data.eibi_language(fields[5].toUtf8().constData());
            StationsItem.strLanguage = QString::fromUtf8(l.c_str());
        }

        if(fieldcount>6)
        {
            string s = fields[6].toUtf8().constData();
            string t = data.eibi_target(s);
            if(t == "")
            {
                string c = data.itu_r_country(s);
                if(c == "")
                    StationsItem.strTarget = QString::fromUtf8(s.c_str());
                else
                    StationsItem.strTarget = QString::fromUtf8(c.c_str());
            }
            else
            {
                StationsItem.strTarget = QString::fromUtf8(t.c_str());
            }
        }
        string country;
        string stn;
        if(fieldcount>7)
        {
            StationsItem.strSite = fields[7];
            string s  = fields[7].toUtf8().constData();
            if(s=="") // unknown or main Tx site of the home country
            {
                country = homecountry;
            }
            else
            {
                size_t i=0;
                s += '-';
                if(s[0]=='/') // transmitted from another country
                    i++;
                string a,b;
                while(s[i]!='-')
                    a += s[i++];
                i++;
                if(i<s.length())
                    while(s[i]!='-')
                        b += s[i++];
                if(s[0]=='/')
                {
                    country = a;
                    stn = b;
                }
                else
                {
                    if(a.length()==3)
                    {
                        country = a;
                        stn = b;
                    }
                    else
                    {
                        country = homecountry;
                        stn = a;
                    }
                }
            }
        }
        else
        {
            country = homecountry;
        }
        QString site = QString::fromUtf8(data.eibi_station(country, stn).c_str());
        if(site == "")
        {
            //cout << StationsItem.iFreq << " [" << StationsItem.strSite << "] [" << country << "] [" << stn << "]" << endl;
        }
        else
        {
            StationsItem.strSite = site;
        }

        /* Add new item in table */
        StationsTable.push_back(StationsItem);

        UpdateStringListForFilter(StationsItem);

    } while(!feof(pFile));
}

Station::EState CDRMSchedule::GetState(const int iPos)
{
    /* Get system time */
    time_t now = time(NULL);
    struct tm tm = *gmtime(&now);
    QDate d(1900+tm.tm_year, tm.tm_mon+1, tm.tm_mday);
    QTime t(tm.tm_hour, tm.tm_min, tm.tm_sec);
    QDateTime dt(d,t);
    return StationsTable[iPos].stateAt(dt, GetSecondsPreview());
}

bool CDRMSchedule::CheckFilter(const int iPos)
{
    const CStationsItem& station = StationsTable[iPos];
    if(eSchedMode==SM_DRM && targetFilterdrm != "" && targetFilterdrm != station.strTarget)
        return false;
    if(eSchedMode==SM_DRM && countryFilterdrm != "" && countryFilterdrm != station.strCountry)
        return false;
    if(eSchedMode==SM_DRM && languageFilterdrm != "" && languageFilterdrm != station.strLanguage)
        return false;
    if(eSchedMode==SM_ANALOG && targetFilteranalog != "" && targetFilteranalog != station.strTarget)
        return false;
    if(eSchedMode==SM_ANALOG && countryFilteranalog != "" && countryFilteranalog != station.strCountry)
        return false;
    if(eSchedMode==SM_ANALOG && languageFilteranalog != "" && languageFilteranalog != station.strLanguage)
        return false;
    return true;
}

Station::EState CStationsItem::stateAt(const QDateTime& ltime, int previewSeconds) const
{
    if (activeAt(ltime) == TRUE)
    {
		QDateTime dt = ltime.addSecs(NUM_SECONDS_SOON_INACTIVE);
	
        /* Check if the station soon will be inactive */
        if (activeAt(dt) == TRUE)
            return Station::IS_ACTIVE;
        else
            return Station::IS_SOON_INACTIVE;
    }
    else
    {
		QDateTime dt = ltime.addSecs(previewSeconds);
		if (activeAt(dt) == TRUE)
		{
				return Station::IS_PREVIEW;
		}
		return Station::IS_INACTIVE;
    }
}

_BOOLEAN CStationsItem::activeAt(const QDateTime& wantedTime) const
{
    QDateTime today = QDateTime(wantedTime.date());
    
    /* Get start time */
    QDateTime start = today.addSecs(60*(60*iStartHour+iStartMinute));

    /* Get stop time */
    QDateTime stop = today.addSecs(60*(60*iStopHour+iStopMinute));

    /* Check, if stop time is on next day */
    if(stop < start)
	stop = stop.addDays(1);

    /* Check day
       tm_wday: day of week (0 - 6; Sunday = 0). "strDaysFlags" are coded with
       pseudo binary representation. A one signals that day is active. The most
       significant 1 is the sunday, then followed the monday and so on. */
    int wday = start.date().dayOfWeek(); // 1=Monday, 7=Sunday
    if(wday==7) wday=0;
    if ((strDaysFlags[wday] == CHR_ACTIVE_DAY_MARKER) ||
            /* Check also for special case: days are 0000000. This is reserved for
               DRM test transmissions or irregular transmissions. We define here
               that these stations are transmitting every day */
            (strDaysFlags == FLAG_STR_IRREGULAR_TRANSM))
    {
	return ((start <= wantedTime) && (wantedTime < stop))?TRUE:FALSE;
    }
    return FALSE;
}

void CStationsItem::SetDaysFlagString(const QString& strNewDaysFlags)
{
    /* Set internal "days flag" string and "show days" string */
    strDaysFlags = strNewDaysFlags;
    strDaysShow = "";

    /* Init days string vector */
    const QString strDayDef [] =
    {
        QObject::tr("Sun"),
        QObject::tr("Mon"),
        QObject::tr("Tue"),
        QObject::tr("Wed"),
        QObject::tr("Thu"),
        QObject::tr("Fri"),
        QObject::tr("Sat")
    };

    /* First test for day constellations which allow to apply special names */
    if (strDaysFlags == FLAG_STR_IRREGULAR_TRANSM)
        strDaysShow = QObject::tr("irregular");
    else if (strDaysFlags == "1111111")
        strDaysShow = QObject::tr("daily");
    else if (strDaysFlags == "1111100")
        strDaysShow = QObject::tr("from Sun to Thu");
    else if (strDaysFlags == "1111110")
        strDaysShow = QObject::tr("from Sun to Fri");
    else if (strDaysFlags == "0111110")
        strDaysShow = QObject::tr("from Mon to Fri");
    else if (strDaysFlags == "0111111")
        strDaysShow = QObject::tr("from Mon to Sat");
    else
    {
        /* No special name could be applied, just list all active days */
        for (int i = 0; i < 7; i++)
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
}

#ifdef HAVE_LIBHAMLIB
StationsDlg::StationsDlg(CDRMReceiver& DRMReceiver, CSettings& Settings, CRig& Rig, QMap<QWidget*,QString>& parents):
    CWindow(parents, Settings, "Stations"),
    DRMReceiver(DRMReceiver), Rig(Rig),
#else
StationsDlg::StationsDlg(CDRMReceiver& DRMReceiver, CSettings& Settings, QMap<QWidget*,QString>& parents):
    CWindow(parents, Settings, "Stations"),
    DRMReceiver(DRMReceiver),
#endif
    greenCube(":/icons/greenCube.png"), redCube(":/icons/redCube.png"),
    orangeCube(":/icons/orangeCube.png"), pinkCube(":/icons/pinkCube.png"),
    bReInitOnFrequencyChange(FALSE), eLastScheduleMode(CDRMSchedule::SM_NONE)
{
    setupUi(this);

    /* Load settings */
    LoadSettings();

    /* Set help text for the controls */
    AddWhatsThisHelp();

    ProgrSigStrength->hide();
    TextLabelSMeter->hide();

    /* Set up frequency selector control (QWTCounter control) */
#if QWT_VERSION < 0x060100
    QwtCounterFrequency->setRange(0.0, MAX_RF_FREQ, 1.0);
#else
    QwtCounterFrequency->setRange(0.0, MAX_RF_FREQ);
    QwtCounterFrequency->setSingleStep(1.0);
#endif
    QwtCounterFrequency->setNumButtons(3); /* Three buttons on each side */
    QwtCounterFrequency->setIncSteps(QwtCounter::Button1, 1); /* Increment */
    QwtCounterFrequency->setIncSteps(QwtCounter::Button2, 10);
    QwtCounterFrequency->setIncSteps(QwtCounter::Button3, 100);

    ListViewStations->setAllColumnsShowFocus(true);
    ListViewStations->setColumnCount(9);
    ListViewStations->setRootIsDecorated ( false );
    ListViewStations->setSortingEnabled ( true );
    QStringList headers;
    headers
            << QString() /* icon, enable sorting by online/offline */
            << tr("Station Name")
            << tr("Time [UTC]")
            << tr("Frequency [kHz]")
            << tr("Power [kW]")
            << tr("Target")
            << tr("Country")
            << tr("Site")
            << tr("Language")
            << tr("Days");
    ListViewStations->setHeaderLabels(headers);
    ListViewStations->headerItem()->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
    ListViewStations->headerItem()->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
    ListViewStations->headerItem()->setTextAlignment(4, Qt::AlignRight | Qt::AlignVCenter);

    previewMapper = new QSignalMapper(this);
    previewGroup = new QActionGroup(this);
    showMapper = new QSignalMapper(this);
    showGroup = new QActionGroup(this);
    showGroup->addAction(actionShowOnlyActiveStations);
    showMapper->setMapping(actionShowOnlyActiveStations, 0);
    showGroup->addAction(actionShowAllStations);
    showMapper->setMapping(actionShowAllStations, 1);
    connect(actionClose, SIGNAL(triggered()), SLOT(close()));
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
    connect(ListViewStations->header(), SIGNAL(sectionClicked(int)), this, SLOT(OnHeaderClicked(int)));

    //connect(actionGetUpdate, SIGNAL(triggered()), this, SLOT(OnGetUpdate()));
# ifdef HAVE_LIBHAMLIB
    RigDlg *pRigDlg = new RigDlg(Rig, this);
    connect(actionChooseRig, SIGNAL(triggered()), pRigDlg, SLOT(show()));
# else
    actionChooseRig->setEnabled(false);
# endif
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));
    connect(actionEnable_S_Meter, SIGNAL(triggered()), this, SLOT(OnSMeterMenu()));

    /* Init progress bar for input s-meter */
    InitSMeter(this, ProgrSigStrength);

    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(OnUrlFinished(QNetworkReply*)));

    /* Connections ---------------------------------------------------------- */

    connect(&TimerList, SIGNAL(timeout()), this, SLOT(OnTimerList()));
    connect(&TimerUTCLabel, SIGNAL(timeout()), this, SLOT(OnTimerUTCLabel()));
    connect(QwtCounterFrequency, SIGNAL(valueChanged(double)),
            this, SLOT(OnFreqCntNewValue(double)));

    okMessage = tr("Update successful.");
    badMessage =
        tr("Update failed. The following things may caused the "
           "failure:\n"
           "\t- the internet connection was not set up properly\n"
           "\t- the server is currently not available\n"
           "\t- the file 'DRMSchedule.ini' could not be written");
}

StationsDlg::~StationsDlg()
{
}

void StationsDlg::OnShowStationsMenu(int iID)
{
    (void)iID;
    /* Update list view */
    SetStationsView();
}

void StationsDlg::OnShowPreviewMenu(int iID)
{
    DRMSchedule.SetSecondsPreview(iID);

    /* Update list view */
    SetStationsView();
}

void StationsDlg::AddUpdateDateTime()
{
    /*
    	Set time and date of current schedule in the menu text for
    	querying a new schedule (online schedule update).
    	If no schedule file exists, do not show any time and date.
    */

    /* init with empty string in case there is no schedule file */
    QString s = "";

    /* get time and date information */
    QFileInfo f = QFileInfo(DRMSchedule.schedFileName);
    if (f.exists()) /* make sure the DRM schedule file exists */
    {
        /* use QT-type of data string for displaying */
        s = tr(" (last update: ")
            + f.lastModified().date().toString() + ")";
    }

    actionGetUpdate->setText(tr("&Get Update") + s + "...");
}

void StationsDlg::on_ComboBoxFilterTarget_activated(const QString& s)
{
    if (DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM)
        DRMSchedule.targetFilterdrm = s;
    else
        DRMSchedule.targetFilteranalog = s;
    SetStationsView();
}

void StationsDlg::on_ComboBoxFilterCountry_activated(const QString& s)
{
    if (DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM)
        DRMSchedule.countryFilterdrm = s;
    else
        DRMSchedule.countryFilteranalog = s;
    SetStationsView();
}

void StationsDlg::on_ComboBoxFilterLanguage_activated(const QString& s)
{
    if (DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM)
        DRMSchedule.languageFilterdrm = s;
    else
        DRMSchedule.languageFilteranalog = s;
    SetStationsView();
}

void StationsDlg::on_actionGetUpdate_triggered()
{
    const bool bDrmMode = DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM;
    QUrl& qurl(bDrmMode ? DRMSchedule.qurldrm : DRMSchedule.qurlanalog);
    if (QMessageBox::information(this, tr("Dream Schedule Update"),
                                 tr("Dream tries to download the newest schedule\n"
                                    "Your computer must be connected to the internet.\n\n"
                                    "The current file will be overwritten.\n"
                                    "Do you want to continue?"),
                                 QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )
    {
        /* Try to download the current schedule. Copy the file to the
           current working directory (which is "QDir().absFilePath(NULL)") */
        manager->get(QNetworkRequest(qurl));
    }
}

void StationsDlg::OnUrlFinished(QNetworkReply* reply)
{
    if(reply->error()==QNetworkReply::NoError)
    {
        QFile f(DRMSchedule.schedFileName);
        if(f.open(QIODevice::WriteOnly)) {
            f.write(reply->readAll());
            f.close();
            /* Notify the user that update was successful */
            QMessageBox::information(this, "Dream", okMessage, QMessageBox::Ok);
            /* Read updated ini-file */
            LoadSchedule();
            /* add last update information on menu item */
            AddUpdateDateTime();
        } else {
            QMessageBox::information(this, "Dream", tr("Can't save new schedule"), QMessageBox::Ok);
        }
    }
    else
    {
        QMessageBox::information(this, "Dream", badMessage, QMessageBox::Ok);
    }
}

void CDRMSchedule::SetAnalogUrl()
{
    QDate d = QDate::currentDate();
    int month = d.month();
    int year = 0;
    char season = 0;

// transitions last sunday in March and October
    switch(month) {
    case 1:
    case 2:
        year = d.year()-1;
        season = 'b';
        break;
    case 3: {
        QDate s = d;
        s.setDate(d.year(), month+1, 1);
        s = s.addDays(0-s.dayOfWeek());
        if(d<s) {
            year = d.year()-1;
            season = 'b';
        } else {
            year = d.year();
            season = 'a';
        }
    }
    break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        year = d.year();
        season = 'a';
        break;
    case 10: {
        QDate s = d;
        s.setDate(d.year(), month+1, 1);
        int n = s.dayOfWeek();
        s = s.addDays(0-n);
        if(d<s) {
            year = d.year();
            season = 'a';
        } else {
            year = d.year();
            season = 'b';
        }
    }
    break;
    case 11:
    case 12:
        year = d.year();
        season = 'b';
    }
    if (season)
        qurlanalog = QUrl(QString("http://eibispace.de/dx/sked-%1%2.csv").arg(season).arg(year-2000,2));
}

void StationsDlg::eventClose(QCloseEvent*)
{
    /* Save settings */
    SaveSettings();
}

void StationsDlg::eventHide(QHideEvent*)
{
    /* Deactivate real-time timers */
    TimerList.stop();
    TimerUTCLabel.stop();
    DisableSMeter();
}

void StationsDlg::eventShow(QShowEvent*)
{
    /* Activate real-time timer when window is shown */
    TimerList.start(GUI_TIMER_LIST_VIEW_STAT); /* Stations list */
    TimerUTCLabel.start(GUI_TIMER_UTC_TIME_LABEL);

    QwtCounterFrequency->setValue(DRMReceiver.GetFrequency());

    bool ensmeter = false;
    ensmeter = actionEnable_S_Meter->isChecked();
    if(ensmeter)
        EnableSMeter();
    else
        DisableSMeter();
    CheckMode();
    if(!QFile::exists(DRMSchedule.schedFileName))
        QMessageBox::information(this, "Dream", tr("The schedule file "
                                 " could not be found or contains no data.\n"
                                 "No stations can be displayed.\n"
                                 "Try to download this file by using the 'Update' menu."),
                                 QMessageBox::Ok);

    /* Force UTC label update */
    OnTimerUTCLabel();

    QTimer::singleShot(1000, this, SLOT(OnTimerList()));
}

void StationsDlg::CheckMode()
{
    CDRMSchedule::ESchedMode eSchedM = DRMSchedule.GetSchedMode();
    ERecMode eRecM = DRMReceiver.GetReceiverMode();
    if (eSchedM == CDRMSchedule::SM_DRM &&  eRecM != RM_DRM)
    {
        DRMSchedule.SetSchedMode(CDRMSchedule::SM_ANALOG);
        AddUpdateDateTime();
    }
    if (eSchedM == CDRMSchedule::SM_ANALOG &&  eRecM == RM_DRM)
    {
        DRMSchedule.SetSchedMode(CDRMSchedule::SM_DRM);
        AddUpdateDateTime();
    }
}

void StationsDlg::OnTimerUTCLabel()
{
    /* Get current UTC time */
    time_t ltime;
    time(&ltime);
    struct tm* gmtCur = gmtime(&ltime);

    /* Generate time in format "UTC 12:00" */
    QString strUTCTime = QString().sprintf("%02d:%02d UTC",
                                           gmtCur->tm_hour, gmtCur->tm_min);

    /* Only apply if time label does not show the correct time */
    if (TextLabelUTCTime->text().compare(strUTCTime))
        TextLabelUTCTime->setText(strUTCTime);
}

void StationsDlg::OnTimerList()
{
    if (isVisible())
    {
        if (DRMSchedule.GetStationNumber() == 0)
        {
            LoadSchedule();
        }

        /* frequency could be changed by evaluation dialog or RSCI */
        int iFrequency = DRMReceiver.GetFrequency();
        int iCurFrequency = QwtCounterFrequency->value();

        if (iFrequency != iCurFrequency)
        {
            QwtCounterFrequency->setValue(iFrequency);
        }

        /* Update list view */
        SetStationsView();
    }
}

void StationsDlg::LoadSettings()
{
    /* S-meter settings */
    bool ensmeter = Settings.Get("Hamlib", "ensmeter", false);

    actionEnable_S_Meter->setChecked(ensmeter);

    bool showAll = getSetting("showall", false);
    int iPrevSecs = getSetting("preview", NUM_SECONDS_PREV_5MIN);
    DRMSchedule.SetSecondsPreview(iPrevSecs);

    if(showAll)
        actionShowAllStations->setChecked(true);
    else
        actionShowOnlyActiveStations->setChecked(true);

    switch (iPrevSecs)
    {
    case NUM_SECONDS_PREV_5MIN:
        action5minutes->setChecked(true);
        break;

    case NUM_SECONDS_PREV_15MIN:
        action15minutes->setChecked(true);
        break;

    case NUM_SECONDS_PREV_30MIN:
        action30minutes->setChecked(true);
        break;

    default: /* case 0, also takes care of out of value parameters */
        actionDisabled->setChecked(true);
        break;
    }

    /* get sorting and filtering behaviour */
    ERecMode eRecSM = DRMReceiver.GetReceiverMode();
    switch (eRecSM)
    {
    case RM_DRM:
        DRMSchedule.SetSchedMode(CDRMSchedule::SM_DRM);
        break;

    case RM_AM:
        DRMSchedule.SetSchedMode(CDRMSchedule::SM_ANALOG);
        break;
    default: // can't happen!
        ;
    }
    iSortColumndrm = getSetting("sortcolumndrm", 0);
    bCurrentSortAscendingdrm = getSetting("sortascendingdrm", true);
    strColumnParamdrm = getSetting("columnparamdrm", QString());
    iSortColumnanalog = getSetting("sortcolumnanalog", 0);
    bCurrentSortAscendinganalog = getSetting("sortascendinganalog", true);
    strColumnParamanalog = getSetting("columnparamanalog", QString());

    DRMSchedule.qurldrm = QUrl(getSetting("DRM URL", QString(DRM_SCHEDULE_URL)));
    DRMSchedule.targetFilterdrm = getSetting("targetfilterdrm", QString());
    DRMSchedule.countryFilterdrm = getSetting("countryfilterdrm", QString());
    DRMSchedule.languageFilterdrm = getSetting("languagefilterdrm", QString());
    DRMSchedule.targetFilteranalog = getSetting("targetfilteranalog", QString());
    DRMSchedule.countryFilteranalog = getSetting("countryfilteranalog", QString());
    DRMSchedule.languageFilteranalog = getSetting("languagefilteranalog", QString());
}

void StationsDlg::SaveSettings()
{
    if (eLastScheduleMode != CDRMSchedule::SM_NONE)
    {
        const bool bDrmMode = eLastScheduleMode == CDRMSchedule::SM_DRM;
        ColumnParamToStr(bDrmMode ? strColumnParamdrm : strColumnParamanalog);
    }

    Settings.Put("Hamlib", "ensmeter", actionEnable_S_Meter->isChecked());
    putSetting("showall", actionShowAllStations->isChecked());
    putSetting("DRM URL", DRMSchedule.qurldrm.toString());
    putSetting("ANALOG URL", DRMSchedule.qurlanalog.toString());
    putSetting("sortcolumndrm", iSortColumndrm);
    putSetting("sortascendingdrm", bCurrentSortAscendingdrm);
    putSetting("columnparamdrm", strColumnParamdrm);
    putSetting("sortcolumnanalog", iSortColumnanalog);
    putSetting("sortascendinganalog", bCurrentSortAscendinganalog);
    putSetting("columnparamanalog", strColumnParamanalog);
    putSetting("targetfilterdrm", DRMSchedule.targetFilterdrm);
    putSetting("countryfilterdrm", DRMSchedule.countryFilterdrm);
    putSetting("languagefilterdrm", DRMSchedule.languageFilterdrm);
    putSetting("targetfilteranalog", DRMSchedule.targetFilteranalog);
    putSetting("countryfilteranalog", DRMSchedule.countryFilteranalog);
    putSetting("languagefilteranalog", DRMSchedule.languageFilteranalog);

    /* Store preview settings */
    putSetting("preview", DRMSchedule.GetSecondsPreview());
}

void StationsDlg::LoadFilters()
{
    ComboBoxFilterTarget->clear();
    ComboBoxFilterCountry->clear();
    ComboBoxFilterLanguage->clear();
    ComboBoxFilterTarget->addItems(DRMSchedule.ListTargets);
    ComboBoxFilterCountry->addItems(DRMSchedule.ListCountries);
    ComboBoxFilterLanguage->addItems(DRMSchedule.ListLanguages);

    QString targetFilter,countryFilter,languageFilter;
    if(DRMSchedule.GetSchedMode()==CDRMSchedule::SM_DRM)
    {
        targetFilter=DRMSchedule.targetFilterdrm;
        countryFilter=DRMSchedule.countryFilterdrm;
        languageFilter=DRMSchedule.languageFilterdrm;
    }
    else
    {
        targetFilter=DRMSchedule.targetFilteranalog;
        countryFilter=DRMSchedule.countryFilteranalog;
        languageFilter=DRMSchedule.languageFilteranalog;
    }
    ComboBoxFilterTarget->setCurrentIndex(ComboBoxFilterTarget->findText(targetFilter));
    ComboBoxFilterCountry->setCurrentIndex(ComboBoxFilterCountry->findText(countryFilter));
    ComboBoxFilterLanguage->setCurrentIndex(ComboBoxFilterLanguage->findText(languageFilter));
}

void StationsDlg::LoadSchedule()
{
    /* Store previous columns settings */
    if (eLastScheduleMode != CDRMSchedule::SM_NONE)
    {
        const bool bDrmMode = eLastScheduleMode == CDRMSchedule::SM_DRM;
        ColumnParamToStr(bDrmMode ? strColumnParamdrm : strColumnParamanalog);
    }

    ClearStationsView();

    DRMSchedule.LoadSchedule();

    int i;

    for (i = 0; i < DRMSchedule.GetStationNumber(); i++)
    {
        const CStationsItem& station = DRMSchedule.GetItem(i);

        /* Get power of the station. We have to do a special treatment
        	 * here, because we want to avoid having a "0" in the list when
        	 * a "?" was in the schedule-ini-file */
        const _REAL rPower = station.rPower;

        QString strPower;
        if (rPower == (_REAL) 0.0)
            strPower = "?";
        else
            strPower.setNum(rPower);

        QString strTimes = QString().sprintf("%04d-%04d", station.StartTime(), station.StopTime());

        /* Generate new list station with all necessary column entries */
        QTreeWidgetItem* item = new CaseInsensitiveTreeWidgetItem(ListViewStations);
        item->setText(1, station.strName);
        item->setText(2, strTimes /* time */);
        item->setText(3, QString().setNum(station.iFreq) /* freq. */);
        item->setText(4, strPower            /* power */);
        item->setText(5, station.strTarget   /* target */);
        item->setText(6, station.strCountry  /* country */);
        item->setText(7, station.strSite     /* site */);
        item->setText(8, station.strLanguage /* language */);
        item->setText(9, station.strDaysShow);
        item->setData(1, Qt::UserRole, i);
        item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);
        item->setTextAlignment(4, Qt::AlignRight | Qt::AlignVCenter);
    }
    int c;
    bool b;
    if(DRMSchedule.GetSchedMode()==CDRMSchedule::SM_DRM)
    {
        b = bCurrentSortAscendingdrm;
        c = iSortColumndrm;
    }
    else
    {
        b = bCurrentSortAscendinganalog;
        c = iSortColumnanalog;
    }
    ListViewStations->sortByColumn(c, b ? Qt::AscendingOrder : Qt::DescendingOrder);
    LoadFilters();

    /* Restore columns settings */
    eLastScheduleMode = DRMSchedule.GetSchedMode();
    const bool bDrmMode = eLastScheduleMode == CDRMSchedule::SM_DRM;
    ColumnParamFromStr(bDrmMode ? strColumnParamdrm : strColumnParamanalog);

    /* Update list view */
    SetStationsView();
}

void StationsDlg::ClearStationsView()
{
    ListViewStations->clear();
}

void StationsDlg::SetStationsView()
{
    TimerList.stop();
    ListItemsMutex.lock();

    bool bShowAll = showAll();
    ListViewStations->setSortingEnabled(false);
    for (int i = 0; i < ListViewStations->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* item = ListViewStations->topLevelItem(i);
        int scheduleItem = item->data(1, Qt::UserRole).toInt();

        Station::EState iState = DRMSchedule.GetState(scheduleItem);

        switch (iState)
        {
        case Station::IS_ACTIVE:
            item->setData(0, Qt::UserRole, 1);
            item->setIcon(0, greenCube);
            break;
        case Station::IS_PREVIEW:
            item->setData(0, Qt::UserRole, 2);
            item->setIcon(0, orangeCube);
            break;
        case Station::IS_SOON_INACTIVE:
            item->setData(0, Qt::UserRole, 0);
            item->setIcon(0, pinkCube);
            break;
        case Station::IS_INACTIVE:
            item->setData(0, Qt::UserRole, 3);
            item->setIcon(0, redCube);
            break;
        default:
            item->setData(0, Qt::UserRole, 4);
            item->setIcon(0, redCube);
            break;
        }
        if(DRMSchedule.CheckFilter(scheduleItem) && (bShowAll || (iState != Station::IS_INACTIVE)))
        {
            item->setHidden(false);
        }
        else
        {
            item->setHidden(true);
        }
    }
    ListViewStations->setSortingEnabled(true);
    ListViewStations->sortItems(ListViewStations->sortColumn(), GetSortAscending()?Qt::AscendingOrder:Qt::DescendingOrder);
    ListViewStations->setFocus();
    ListItemsMutex.unlock();
    TimerList.start(GUI_TIMER_LIST_VIEW_STAT);
}

void StationsDlg::OnFreqCntNewValue(double dVal)
{
    DRMReceiver.SetFrequency(int(dVal));
}

void StationsDlg::OnHeaderClicked(int c)
{
    /* Store the "direction" of sorting */
    if (currentSortColumn() == c)
        SetSortAscending(!GetSortAscending());
    else
        SetSortAscending(true);
    /* Store the column of sorting */
    if (DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM)
        iSortColumndrm = c;
    else
        iSortColumnanalog = c;
}

int StationsDlg::currentSortColumn()
{
    if (DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM)
        return iSortColumndrm;
    else
        return iSortColumnanalog;
}

void StationsDlg::SetSortAscending(_BOOLEAN b)
{
    if (DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM)
        bCurrentSortAscendingdrm = b;
    else
        bCurrentSortAscendinganalog = b;
}

_BOOLEAN StationsDlg::GetSortAscending()
{
    if (DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM)
        return bCurrentSortAscendingdrm;
    else
        return bCurrentSortAscendinganalog;
}

void StationsDlg::ColumnParamFromStr(const QString& strColumnParam)
{
    QStringList list(strColumnParam.split(QChar('|')));
    const int n = list.count(); /* width and position */
    if (n == 2)
    {
        for (int j = 0; j < n; j++)
        {
            int c = ListViewStations->header()->count();
            QStringList values(list[j].split(QChar(',')));
            const int lc = (int)values.count();
            if (lc < c)
                c = lc;
            for (int i = 0; i < c; i++)
            {
                int v = values[i].toInt();
                if (!j) /* width*/
                    ListViewStations->header()->resizeSection(i, v);
                else /* position */
                    ListViewStations->header()->moveSection(ListViewStations->header()->visualIndex(i), v);
            }
        }
    }
    else
    {
        ListViewStations->header()->resizeSections(QHeaderView::ResizeToContents);
        ListViewStations->header()->resizeSections(QHeaderView::Interactive);
        ListViewStations->header()->resizeSection(0, ListViewStations->header()->minimumSectionSize());
    }
}

void StationsDlg::ColumnParamToStr(QString& strColumnParam)
{
    strColumnParam = "";
    const int n = 2; /* width and position */
    for (int j = 0; j < n; j++)
    {
        const int c = ListViewStations->header()->count();
        for (int i = 0; i < c; i++)
        {
            int v;
            if (!j) /* width*/
                v = ListViewStations->header()->sectionSize(i);
            else /* position */
                v = ListViewStations->header()->visualIndex(i);
            QString strValue;
            strValue.setNum(v);
            strColumnParam += strValue;
            if (i < (c-1))
                strColumnParam += ",";
        }
        if (j < (n-1))
            strColumnParam += "|";
    }
}

void StationsDlg::SetFrequencyFromGUI(int iFreq)
{
    QwtCounterFrequency->setValue(iFreq);

    /* If the mode has changed re-initialise the receiver */
    ERecMode eCurrentMode = DRMReceiver.GetReceiverMode();

    /* if "bReInitOnFrequencyChange" is not true, initiate a reinit when
     schedule mode is different from receiver mode */
    switch (DRMSchedule.GetSchedMode())
    {
    case CDRMSchedule::SM_NONE:
        break;

    case CDRMSchedule::SM_DRM:
        if (eCurrentMode != RM_DRM)
            DRMReceiver.SetReceiverMode(RM_DRM);
        if (bReInitOnFrequencyChange)
            DRMReceiver.RequestNewAcquisition();
        break;

    case CDRMSchedule::SM_ANALOG:
        if (eCurrentMode != RM_AM)
            DRMReceiver.SetReceiverMode(RM_AM);
        if (bReInitOnFrequencyChange)
            DRMReceiver.RequestNewAcquisition();
        break;
    }
}

void StationsDlg::on_ListViewStations_itemSelectionChanged()
{
    QList<QTreeWidgetItem *> items =  ListViewStations->selectedItems();
    if(items.size()==1)
    {
        SetFrequencyFromGUI(QString(items.first()->text(3)).toInt());
    }
}

void StationsDlg::OnSMeterMenu(int iID)
{
    (void)iID;
}

void StationsDlg::OnSMeterMenu()
{
    if(actionEnable_S_Meter->isChecked())
        EnableSMeter();
    else
        DisableSMeter();
}

void StationsDlg::EnableSMeter()
{
    TextLabelSMeter->setEnabled(TRUE);
    ProgrSigStrength->setEnabled(TRUE);
    TextLabelSMeter->show();
    ProgrSigStrength->show();
    emit subscribeRig();
}

void StationsDlg::DisableSMeter()
{
    TextLabelSMeter->hide();
    ProgrSigStrength->hide();
    emit unsubscribeRig();
}

void StationsDlg::OnSigStr(double rCurSigStr)
{
    ProgrSigStrength->setValue(rCurSigStr);
}

void StationsDlg::AddWhatsThisHelp()
{
    /* Stations List */
    QString strList =
        tr("<b>Stations List:</b> In the stations list "
           "view all DRM stations which are stored in the DRMSchedule.ini file "
           "are shown. It is possible to show only active stations by changing a "
           "setting in the 'view' menu. The color of the cube on the left of a "
           "menu item shows the current status of the DRM transmission. A green "
           "box shows that the transmission takes place right now, a "
//           "yellow cube shows that this is a test transmission and with a "
           "red cube it is shown that the transmission is offline, "
           "a pink cube shown that the transmission soon will be offline.<br>"
           "If the stations preview is active an orange box shows the stations "
           "that will be active.<br>"
           "The list can be sorted by clicking on the headline of the "
           "column.<br>By clicking on a menu item, a remote front-end can "
           "be automatically switched to the current frequency and the "
           "Dream software is reset to a new acquisition (to speed up the "
           "synchronization process). Also, the log-file frequency edit "
           "is automatically updated.");

    /* Frequency Counter */
    QString strCounter =
        tr("<b>Frequency Counter:</b> The current frequency "
           "value can be changed by using this counter. The tuning steps are "
           "100 kHz for the  buttons with three arrows, 10 kHz for the "
           "buttons with two arrows and 1 kHz for the buttons having only "
           "one arrow. By keeping the button pressed, the values are "
           "increased / decreased automatically.");

    /* UTC time label */
    QString strTime =
        tr("<b>UTC Time:</b> Shows the current Coordinated "
           "Universal Time (UTC) which is also known as Greenwich Mean Time "
           "(GMT).");

    /* S-meter */
    const QString strSMeter =
        tr("<b>Signal-Meter:</b> Shows the signal strength "
           "level in dB relative to S9.<br>Note that not all "
           "front-ends controlled by hamlib support this feature. If the s-meter "
           "is not available, the controls are disabled.");

    ListViewStations->setWhatsThis(strList);
    QwtCounterFrequency->setWhatsThis(strCounter);
    TextLabelUTCTime->setWhatsThis(strTime);
    TextLabelSMeter->setWhatsThis(strSMeter);
    ProgrSigStrength->setWhatsThis(strSMeter);
}

_BOOLEAN StationsDlg::showAll()
{
    return actionShowAllStations->isChecked();
}

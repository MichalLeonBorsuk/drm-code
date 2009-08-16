/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	ETSI DAB/DRM Electronic Programme Guide Viewer
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

#include "EPGDlg.h"
#include <QTextStream>
#include <QRegExp>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <ctime>

EPGModel::EPGModel(CParameter& p) : EPG(p), QAbstractTableModel(),
BitmCubeGreen(8, 8)
{
	BitmCubeGreen.fill(QColor(0, 255, 0));
}

int EPGModel::rowCount(const QModelIndex&) const
{
	return progs.size();
}

int EPGModel::columnCount(const QModelIndex&) const
{
	return 5;
}

QVariant
EPGModel::data ( const QModelIndex& index, int role) const
{
	QMap <shortCRIDType, ProgrammeType>::const_iterator it = progs.begin();
	for(int i=0; i<index.row(); i++)
		it++;
	const ProgrammeType& p = it.value();
	QString name, description, genre;
	time_t start;
	int duration;
	if(p.location.size()==0)
		return QVariant(); // TODO better than this
	const LocationType& loc = p.location[0];
	if(loc.time.size()==0)
		return QVariant(); // TODO better than this
	const EPGTime& t = loc.time[0];

	switch(role)
	{
	case Qt::DecorationRole:
		if(index.column()==0)
		{
			if(t.actualTime!=0)
			{
				duration = t.actualDuration;
			}
			else
			{
				duration = t.duration;
			}
			time_t start = it.key();
			time_t end = start+60*duration;
			time_t now = time(NULL);
			if((start <= now) && (now <= end))
			{
				QIcon icon;
				icon.addPixmap(BitmCubeGreen);
				return icon;
			}
			return QVariant();
		}
		break;
	case Qt::DisplayRole:
		switch(index.column())
		{
			case 0:
				// TODO - let user choose time or actualTime if available, or show as tooltip
				{
				    if(t.actualTime!=0)
				    {
					start = t.actualTime;
				    }
				    else
				    {
					start = t.time;
				    }
				    tm bdt = *gmtime(&start);
				    QChar fill('0');
				    return QString("%1:%2").arg(bdt.tm_hour,2,10,fill).arg(bdt.tm_min,2,10,fill);
				}
			break;
			case 1:
				if(p.mediumName.size()>0)
					return p.mediumName[0].text;
				if(p.genre.size()>0)
					return "unknown " + p.genre[0].name.text + " programme";
				return "";
				break;
			case 2:
				description = p.mediaDescription[0].longDescription[0].text;
				if(description=="")
					description = p.mediaDescription[0].shortDescription[0].text;
				// collapse white space in description
				description.replace(QRegExp("[\t\r\n ]+"), " ");
				return description;
				break;
			case 3:
				if(p.genre.size()==0)
					genre = "";
				else
				{
					QString sep="";
					for(size_t i=0; i<p.genre.size(); i++) {
						if(p.genre[i].name.text != "Audio only") {
							genre = genre+sep+p.genre[i].name.text;
							sep = ", ";
						}
					}
				}
				return genre;
				break;
			case 4:
				if(t.actualTime!=0)
				{
					duration = t.actualDuration;
				}
				else
				{
					duration = t.duration;
				}
				ushort hours = duration/60;
				ushort mins = duration % 60;
				QChar fill='0';
				return QString("%1:%2").arg(hours, 2,10,fill).
							arg(mins,2,10,fill);
				break;
		}
		break;
	}
	return QVariant();
}

QVariant
EPGModel::headerData ( int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole)
		return QVariant();
	if(orientation != Qt::Horizontal)
		return QVariant();
	switch(section)
	{
		case 0: return tr("Time [UTC]"); break;
		case 1: return tr("Name"); break;
		case 2: return tr("Description"); break;
		case 3: return tr("Genre"); break;
		case 4: return tr("Duration"); break;
	}
	return "";
}

bool EPGModel::IsActive(const QString& start, const QString& duration, const tm& now)
{
    QStringList sl = start.split(":");
    QStringList dl = duration.split(":");
    int s = 60*sl[0].toInt()+sl[1].toInt();
    int e = s + 60*dl[0].toInt()+dl[1].toInt();
    int n = 60*now.tm_hour+now.tm_min;
	if ((s <= n) && (n < e))
		return true;
	else
		return false;
}

void
EPGModel::select (const uint32_t chan, const QDate & date)
{
    // get schedule for date +/- 1 - will allow user timezones sometime
    min_time = QDateTime(date).toTime_t();
    max_time = min_time+86400;
    for(int i=-1; i<=1; i++)
    {
        QDomDocument doc;
	QDate o = date.addDays(i);
	doc = getFile (o, chan, false);
	parseDoc(doc);
	doc = getFile (o, chan, true);
	parseDoc(doc);
    }
    reset();
}

EPGDlg::EPGDlg(ReceiverInterface& NDRMR, CSettings& NSettings, QWidget* parent, Qt::WFlags f)
:QDialog(parent, f),Ui_EPGDlg(),
date(QDate::currentDate()),
epg(*NDRMR.GetParameters()),DRMReceiver(NDRMR),
Settings(NSettings),Timer(),currentSID(0),proxyModel()
{
    setupUi(this);

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(&epg);
	proxyModel->setFilterKeyColumn(0); // actually we don't care
	proxyModel->setFilterRole(Qt::UserRole);
	proxyModel->setDynamicSortFilter(true);

	tableView->setModel(proxyModel);

	/* Connections ---------------------------------------------------------- */
	connect(channel, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(selectChannel(const QString &)));
	connect(dateEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(setDate(const QDate&)));
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(pushButtonPrev, SIGNAL(clicked()), this, SLOT(OnPrev()));
	connect(pushButtonNext, SIGNAL(clicked()), this, SLOT(OnNext()));
	connect(pushButtonClearCache, SIGNAL(clicked()), this, SLOT(OnClearCache()));
	connect(pushButtonSave, SIGNAL(clicked()), this, SLOT(OnSave()));
	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	/* Deactivate real-time timer */
	Timer.stop();


	/* TODO show a label if EPG decoding is disabled */
	//if (DRMReceiver.GetDataDecoder()->GetDecodeEPG() == true)
	if(true)
		TextEPGDisabled->hide();
	else
		TextEPGDisabled->show();
    dateEdit->setCalendarPopup(true);
}

EPGDlg::~EPGDlg()
{
}

void EPGDlg::OnTimer()
{
    /* Get current UTC time */
    time_t ltime;
    time(&ltime);
    tm gmtCur = *gmtime(&ltime);

    if(gmtCur.tm_sec==0) // minute boundary
    {
	select();
    }
}

void EPGDlg::showEvent(QShowEvent *)
{
    CParameter& Parameters = *DRMReceiver.GetParameters();

	/* recover window size and position */
	CWinGeom s;
	Settings.Get("EPG Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

    /* restore selected service */
    bool ok=false;
    currentSID = QString(Settings.Get("EPG Dialog", "serviceid", string("0")).c_str()).toULong(&ok, 16);
    if(!ok)
        currentSID=0;
    // update the channels combobox from the epg
    channel->clear();
    int n = -1, m = -1;
	Parameters.Lock();
    for (map < uint32_t, CServiceInformation >::const_iterator i = Parameters.ServiceInformation.begin();
         i != Parameters.ServiceInformation.end(); i++) {
		QString channel_label = QString().fromUtf8(i->second.label.begin()->c_str());
		uint32_t channel_id = i->second.id;
    	channel->insertItem(++n, channel_label, channel_id);
    	if(channel_id == currentSID)
    	{
    	    m = n;
    	}
    }
	Parameters.Unlock();
	if(m>=0)
        channel->setCurrentIndex(m);

    date = QDate::currentDate();
    dateEdit->setDate(date);

    epg.progs.clear();
    select();

	/* Activate real-time timer when window is shown */
	Timer.start(GUI_TIMER_EPG_UPDATE);
}

void EPGDlg::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timer */
	Timer.stop();

	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("EPG Dialog", s);
    Settings.Put("EPG Dialog", "serviceid", QString("%1").arg(currentSID, 16).toStdString());
}

void EPGDlg::setDate(const QDate& d)
{
    date = d;
    epg.progs.clear();
    select();
}

void EPGDlg::selectChannel(const QString &)
{
    epg.progs.clear();
    select();
}

void EPGDlg::OnPrev()
{
    dateEdit->setDate(dateEdit->date().addDays(-1));
}

void EPGDlg::OnNext()
{
    dateEdit->setDate(dateEdit->date().addDays(1));
}

void EPGDlg::OnClearCache()
{
    QDir dir(epg.dir);

	/* Check if the directory exists */
	if (!dir.exists())
	{
		return;
	}
	dir.setFilter(QDir::Files | QDir::NoSymLinks);

	QStringList filters;
	filters << "*.EHA" << "*.EHB";
	dir.setNameFilters(filters);

	const QFileInfoList& list = dir.entryInfoList();

	int n=0;
	for(QFileInfoList::const_iterator fi = list.begin();
		fi != list.end(); fi++)
	{
		/* Is a file so remove it */
		dir.remove(fi->fileName());
		n++;
	}
	QMessageBox msgBox(this);
	 msgBox.setText(QString(tr("deleted %1 files")).arg(n));
	 msgBox.exec();
}


void EPGDlg::OnSave()
{
	QStringList filters;
	filters << tr("ETSI XML (*.xml)");
	if(pages->currentIndex()==0)
	{
		filters << tr("Web Page (*.html)") << tr("Comma Separated (*.csv)");
	}
	QFileDialog dialog(this);
	dialog.setNameFilters(filters);
	dialog.setDirectory(epg.dir);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.exec();
	QStringList l = dialog.selectedFiles();
	if(l.size()==1)
	{
		QString nf = dialog.selectedNameFilter();
		QString fn = l[0];
		QString text;
		switch(pages->currentIndex())
		{
		case 0: // grid
			if(nf.contains(".xml"))
			{
				fn += ".xml";
				QDomDocument b,a;
				bool bb = b.setContent(basic->toPlainText());
				bool ba = a.setContent(advanced->toPlainText());
				if(bb && ba)
				{
					// TODO merge(b, a);
					text = b.toString();
				}
				else
				{
					if(bb)
						text = basic->toPlainText();
					else if(ba)
						text = advanced->toPlainText();
					else
						text = "";
				}
			}
			if(nf.contains(".html"))
			{
				fn += ".html";
				text = epg.toHTML();
			}
			if(nf.contains(".csv"))
			{
				fn += ".csv";
				text = epg.toCSV();
			}
			break;
		case 1: // basic
			fn += ".xml";
			text = basic->toPlainText();
			break;
		case 2: // advanced
			fn += ".xml";
			text = advanced->toPlainText();
			break;
		}
		QFile data(fn);
		if (data.open(QFile::WriteOnly | QFile::Truncate))
		{
		     QTextStream out(&data);
		     out << text;
		}
		data.close();
	}
	else
	{
	}
}

void EPGDlg::updateXML(const QDate& date, uint32_t sid, bool adv)
{
    QString def;
    QTextBrowser* browser;
    if(adv)
    {
	def = tr("no advanced profile data");
	browser = advanced;
    }
    else
    {
	def = tr("no basic profile data");
	browser = basic;
    }
    QString xml;
    QDomDocument doc;
    doc = epg.getFile (date, sid, adv);
    epg.parseDoc(doc);
    xml = doc.toString();
    if (xml.length() > 0)
    {
        browser->setText(xml);
    }
    else
    {
    	browser->setText(def);
    }
}

void EPGDlg::select()
{
    currentSID = channel->itemData(channel->currentIndex()).toUInt();
    epg.select(currentSID, date);
    updateXML(date, currentSID, false);
    updateXML(date, currentSID, true);
}

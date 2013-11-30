/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	
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

#include "Util.h"
#include "../DrmTransceiver.h"
#include "../datadecoding/DataDecoder.h"
#include <QRegExp>
#include <QDate>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

/* Ensure that the given filename is secure */
QString VerifyFilename(QString filename)
{
    filename.replace(QRegExp("/"), "_"); /* replace unix-like path separator with '_' */
#ifdef _WIN32
    filename.replace(QRegExp("\\\\"), "_"); /* replace windows path separator with '_' */
    filename.replace(QRegExp(":"), "_"); /* replace ':' with '_' */
#endif
    return filename;
}

/* Ensure that the given html path is secure */
QString VerifyHtmlPath(QString path)
{
    if (path == "..")
        return "_";
#ifdef _WIN32
    path.replace(QRegExp("\\\\"), "_"); /* replace windows path separator with '_' */
    path.replace(QRegExp(":"), "_"); /* replace ':' with '_' */
#endif
    path.replace(QRegExp("^\\.\\./"), "_/"); /* replace '../' at the beginning with '_/' */
    path.replace(QRegExp("/\\.\\.$"), "/_"); /* replace '/..' at the end with '/_' */
    path.replace(QRegExp("/\\.\\./"), "/_/"); /* replace '/../' with '/_/' */
    return path;
}

/* Accept both absolute and relative url, but only return the path component.
   Invalid characters in path are percent-encoded (e.g. space = %20) */
QString UrlEncodePath(QString url)
{
    /* Get path component */
    QString path(QUrl(url, QUrl::TolerantMode).path());
    /* Prepend with '/' if none present */
    if (path.size() == 0 || path.at(0) != QChar('/'))
        path.insert(0, QChar('/'));
    /* Replace multiple '/' by single '/' */
    path.replace(QRegExp("/{1,}"), "/");
    /* Replace all occurrence of '/./' with '/' */
    while (path.indexOf("/./") != -1)
        path.replace(QRegExp("/\\./"), "/");
    /* The Actual percent encoding */
    path = QString(QUrl(path, QUrl::TolerantMode).toEncoded(
        QUrl::RemoveScheme | QUrl::RemoveAuthority |
        QUrl::RemoveQuery | QUrl::RemoveFragment));
    return path;
}

/* Determine if the given url is a directory */
bool IsUrlDirectory(QString url)
{
    return url.endsWith(QChar('/'));
}

/* Convert all www. or http:// or email to real
   clickable link, for use with QLabel and such.
   Code by David Flamand */
QString& Linkify(QString& text, QString linkColor)
{
    int i, j, posWWW=-2, posHTTP=-2, posMAIL=-2, posBegin, posEnd, size;
    if (!linkColor.isEmpty())
        linkColor = " style=\"color: " + linkColor + ";\"";
    size = text.size();
    for (i = 0; i < size;)
    {
        if (posWWW != -1 && posWWW < i)
            posWWW  = text.indexOf("www.", i, Qt::CaseInsensitive);
        if (posHTTP != -1 && posHTTP < i)
            posHTTP = text.indexOf("http://", i, Qt::CaseInsensitive);
        if (posMAIL != -1 && posMAIL < i)
            posMAIL = text.indexOf(QRegExp("\\b[0-9a-z._-]+@[0-9a-z.-]+\\.[a-z]{2,4}\\b", Qt::CaseInsensitive), i);
        if (posMAIL>=0 && (posMAIL<=posWWW || posWWW<0) && (posMAIL<posHTTP || posHTTP<0))
            posBegin = posMAIL;
        else if (posWWW>=0 && (posWWW<posHTTP || posHTTP<0))
            posBegin = posWWW;
        else
            posBegin = posHTTP;
        if (posBegin >= 0)
        {
            posEnd = size;
            for (j = posBegin; j < size; j++)
            {
                int chr = text[j].unicode();
                if (!((chr=='@' && posBegin==posMAIL) ||
                      chr=='.' || chr=='/' ||
                      chr=='~' || chr=='-' ||
                      chr=='_' || chr==':' ||
                     (chr>='a' && chr<='z') ||
                     (chr>='A' && chr<='Z') ||
                     (chr>='0' && chr<='9')))
                {
                    posEnd = j;
                    break;
                }
            }
            const int rawLinkSize = posEnd-posBegin;
            QStringRef rawLink(&text, posBegin, rawLinkSize);
            QString newLink;
            if (posBegin == posMAIL)
                newLink = "<a href=\"mailto:%1\"" + linkColor + ">%1</a>";
            else if (posBegin == posWWW)
                newLink = "<a href=\"http://%1\"" + linkColor + ">%1</a>";
            else /* posBegin == posHTTP */
                newLink = "<a href=\"%1\"" + linkColor + ">%1</a>";
            newLink = newLink.arg(rawLink.toString());
            const int newLinkSize = newLink.size();
            text.replace(posBegin, rawLinkSize, newLink);
            const int diff = newLinkSize - rawLinkSize;
            i = posEnd + diff;
            size += diff;
            if (posWWW >= 0)
                posWWW += diff;
            if (posHTTP >= 0)
                posHTTP += diff;
            if (posMAIL >= 0)
                posMAIL += diff;
        }
        else
            break;
    }
    return text;
}

void CreateDirectories(const QString& strFilename)
{
    /*
        This function is for creating a complete directory structure to a given
    	file name. If there is a pathname like this:
    	/html/files/images/myimage.gif
    	this function create all the folders into MOTCache:
    	/html
    	/html/files
    	/html/files/images
    	QFileInfo only creates a file if the pathname is valid. If not all folders
    	are created, QFileInfo will not save the file. There was no QT function
    	or a hint the QT mailing list found in which does the job of this function.
    */
    for (int i = 0;; i++)
    {
#ifdef _WIN32
        int i1 = strFilename.indexOf(QChar('/'), i);
        int i2 = strFilename.indexOf(QChar('\\'), i);
        i = (i1 >= 0 && ((i1 < i2) || (i2<0))) ? i1 : i2;
#else
        i = strFilename.indexOf(QChar('/'), i);
#endif
        if (i < 0)
            break;
        const QString strDirName = strFilename.left(i);
        if (!strDirName.isEmpty() && !QFileInfo(strDirName).exists())
            QDir().mkdir(strDirName);
    }
}

void RestartTransceiver(CDRMTransceiver *DRMTransceiver)
{
    if (DRMTransceiver != NULL)
    {
        QMutex sleep;
        CParameter& Parameters = *DRMTransceiver->GetParameters();
        DRMTransceiver->Restart();
        while (Parameters.eRunState == CParameter::RESTART)
        {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            sleep.lock(); /* TODO find a better way to sleep on Qt */
            sleep.tryLock(10); /* 10 ms */
            sleep.unlock();
        }
    }
}

QString GetCodecString(const CService& service)
{
    QString strReturn;

    /* First check if it is audio or data service */
    if (service.eAudDataFlag == CService::SF_AUDIO)
    {
        /* Audio service */
        const CAudioParam::EAudSamRat eSamRate = service.AudioParam.eAudioSamplRate;

        /* Audio coding */
        switch (service.AudioParam.eAudioCoding)
        {
        case CAudioParam::AC_NONE:
            break;

        case CAudioParam::AC_AAC:
            /* Only 12 and 24 kHz sample rates are supported for AAC encoding */
            if (eSamRate == CAudioParam::AS_12KHZ)
                strReturn = "aac";
            else
                strReturn = "AAC";
            break;

        case CAudioParam::AC_CELP:
            /* Only 8 and 16 kHz sample rates are supported for CELP encoding */
            if (eSamRate == CAudioParam::AS_8_KHZ)
                strReturn = "celp";
            else
                strReturn = "CELP";
            break;

        case CAudioParam::AC_HVXC:
            strReturn = "HVXC";
            break;

        case CAudioParam::AC_OPUS:
            strReturn = "OPUS ";
            /* Opus Audio sub codec */
            switch (service.AudioParam.eOPUSSubCod)
            {
                case CAudioParam::OS_SILK:
                    strReturn += "SILK ";
                    break;
                case CAudioParam::OS_HYBRID:
                    strReturn += "HYBRID ";
                    break;
                case CAudioParam::OS_CELT:
                    strReturn += "CELT ";
                    break;
            }
            /* Opus Audio bandwidth */
            switch (service.AudioParam.eOPUSBandwidth)
            {
                case CAudioParam::OB_NB:
                    strReturn += "NB";
                    break;
                case CAudioParam::OB_MB:
                    strReturn += "MB";
                    break;
                case CAudioParam::OB_WB:
                    strReturn += "WB";
                    break;
                case CAudioParam::OB_SWB:
                    strReturn += "SWB";
                    break;
                case CAudioParam::OB_FB:
                    strReturn += "FB";
                    break;
            }
        }

        /* SBR */
        if (service.AudioParam.eSBRFlag == CAudioParam::SB_USED)
        {
            strReturn += "+";
        }
    }
    else
    {
        /* Data service */
        strReturn = "Data:";
    }

    return strReturn;
}

QString GetTypeString(const CService& service)
{
    QString strReturn;

    /* First check if it is audio or data service */
    if (service.eAudDataFlag == CService::SF_AUDIO)
    {
        /* Audio service */
        switch (service.AudioParam.eAudioCoding)
        {
        case CAudioParam::AC_NONE:
            break;

        case CAudioParam::AC_OPUS:
            /* Opus channels configuration */
            switch (service.AudioParam.eOPUSChan)
            {
            case CAudioParam::OC_MONO:
            strReturn = "MONO";
            break;

            case CAudioParam::OC_STEREO:
            strReturn = "STEREO";
            break;
            }
            break;

        default:
            /* Mono-Stereo */
            switch (service.AudioParam.eAudioMode)
            {
            case CAudioParam::AM_MONO:
                strReturn = "Mono";
                break;

            case CAudioParam::AM_P_STEREO:
                strReturn = "P-Stereo";
                break;

            case CAudioParam::AM_STEREO:
                strReturn = "Stereo";
                break;
            }
        }
    }
    else
    {
        strReturn = GetDataTypeString(service);
    }
    return strReturn;
}

QString GetDataTypeString(const CService& service)
{
    QString strReturn;
    /* Data service */
    if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
    {
        if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
        {
            switch (service.DataParam.iUserAppIdent)
            {
            case 1:
                strReturn = QObject::tr("Dynamic labels");
                break;

            case DAB_AT_MOTSLIDESHOW:
                strReturn = QObject::tr("MOT Slideshow");
                break;

            case DAB_AT_BROADCASTWEBSITE:
                strReturn = QObject::tr("MOT WebSite");
                break;

            case DAB_AT_TPEG:
                strReturn = QObject::tr("TPEG");
                break;

            case DAB_AT_DGPS:
                strReturn = QObject::tr("DGPS");
                break;

            case DAB_AT_TMC:
                strReturn = QObject::tr("TMC");
                break;

            case DAB_AT_EPG:
                strReturn = QObject::tr("EPG - Electronic Programme Guide");
                break;

            case DAB_AT_JAVA:
                strReturn = QObject::tr("Java");
                break;

            case DAB_AT_JOURNALINE: /* Journaline */
                strReturn = QObject::tr("Journaline");
                break;
            }
        }
        else
            strReturn = QObject::tr("Unknown Service");
    }
    else
        strReturn = QObject::tr("Unknown Service");

    return strReturn;
}

#ifdef QT_GUI_LIB
void SetStatus(CMultColorLED* LED, ETypeRxStatus state)
{
    switch(state)
    {
    case NOT_PRESENT:
        LED->Reset(); /* GREY */
        break;

    case CRC_ERROR:
        LED->SetLight(CMultColorLED::RL_RED);
        break;

    case DATA_ERROR:
        LED->SetLight(CMultColorLED::RL_YELLOW);
        break;

    case RX_OK:
        LED->SetLight(CMultColorLED::RL_GREEN);
        break;
    }
}
#endif

QString getAMScheduleUrl()
{
    QDate d = QDate::currentDate();
    int month = d.month();
    int year = 0;
    char season = 'a';

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
    return QString("http://eibispace.de/dx/sked-%1%2.csv").arg(season).arg(year-2000,2);
}



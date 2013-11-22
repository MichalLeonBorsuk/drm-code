#include "dreamtabwidget.h"
#include <QLabel>
#include <QVariant>
#include <QTabBar>
#include "../Parameter.h"
#include <../util-QT/Util.h>
#include "journalineviewer.h"
#include "bwsviewerwidget.h"
#include "slideshowwidget.h"
#include "EPGDlg.h"
#include <../datadecoding/DataDecoder.h>

DreamTabWidget::DreamTabWidget(QWidget *parent) :
    QTabWidget(parent)
{
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(on_currentChanged(int)));
}

void DreamTabWidget::onServiceChanged(int short_id, const CService& service)
{
    if(service.strLabel!="")
    {
        if(count()<short_id+1)
        {
            if(service.eAudDataFlag==CService::SF_AUDIO)
            {
                addAudioTab(short_id, service);
                if(service.DataParam.iStreamID!=STREAM_ID_NOT_USED)
                    addDataTab(short_id, service, service.iServiceID);
            }
            else
            {
                addDataTab(short_id, service, -1);
            }
        }
        else
        {
            // TODO
        }
    }
}

void DreamTabWidget::on_currentChanged(int index)
{
    int short_id = tabBar()->tabData(index).toInt();
    if(short_id<4)
        emit audioServiceSelected(short_id);
    else
        emit dataServiceSelected(short_id-4);
}

void DreamTabWidget::setText(int short_id, QString text)
{
    for(int i=0; i<count(); i++)
    {
        if(short_id==tabBar()->tabData(i).toInt())
            ((QLabel*)widget(i))->setText(text);
    }
}

void DreamTabWidget::addAudioTab(int short_id, const CService& service)
{
    QString l = QString::fromUtf8(service.strLabel.c_str());
    int index = addTab(new QLabel(QString("short id %1").arg(short_id)), l);
    tabBar()->setTabData(index, short_id);
}

void DreamTabWidget::addDataTab(int short_id, const CService& service, int iAudioServiceID)
{
    QString l = QString::fromUtf8(service.strLabel.c_str());
    if(iAudioServiceID>=0)
    {
        l = l + " " + GetDataTypeString(service);
    }
    QLabel* defaultApp = new QLabel(QString("short id %1 stream %2 packet id %3")
                                   .arg(short_id)
                                   .arg(service.DataParam.iStreamID)
                                   .arg(service.DataParam.iPacketID));
    QWidget* pApp = defaultApp;
    QObject* pController = parent();
    QString wantedName = "DRMMainWindow";
    // for now, the main window will be sending us update events.
    while(pController && pController->objectName()!=wantedName)
        pController = pController->parent();
    if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
    {
        if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
        {
            switch (service.DataParam.iUserAppIdent)
            {
            case DAB_AT_MOTSLIDESHOW:
            {
                SlideShowWidget* p = new SlideShowWidget();
                p->setServiceInformation(short_id, service);
                //p->setSavePath(QString::fromUtf8(Parameters.GetDataDirectory("Journaline").c_str()));
                connect(pController, SIGNAL(dataStatusChanged(int, ETypeRxStatus)), p, SLOT(setStatus(int, ETypeRxStatus)));
                pApp = p;
            }
                break;

            case DAB_AT_BROADCASTWEBSITE:
            {
                BWSViewerWidget* p = new BWSViewerWidget();
                p->setDecoder(service.DataParam.pDecoder);
                p->setServiceInformation(short_id, service);
                //p->setSavePath(QString::fromUtf8(Parameters.GetDataDirectory("Journaline").c_str()));
                connect(pController, SIGNAL(dataStatusChanged(int, ETypeRxStatus)), p, SLOT(setStatus(int, ETypeRxStatus)));
                pApp = p;
            }
                break;

            case DAB_AT_EPG:
                break;

            case DAB_AT_JOURNALINE:
            {
                JournalineViewer* p = new JournalineViewer(short_id);
                p->setDecoder(service.DataParam.pDecoder);
                p->setServiceInformation(service, iAudioServiceID);
                //p->setSavePath(QString::fromUtf8(Parameters.GetDataDirectory("Journaline").c_str()));
                connect(pController, SIGNAL(dataStatusChanged(int, ETypeRxStatus)), p, SLOT(setStatus(int, ETypeRxStatus)));
                pApp = p;
            }
            break;

            default:
                ;
            }
        }
    }
    int index = addTab(pApp, l);
    tabBar()->setTabData(index, 4+short_id);
}

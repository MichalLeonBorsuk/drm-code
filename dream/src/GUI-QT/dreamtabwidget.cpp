#include "dreamtabwidget.h"
#include <QLabel>
#include <QVariant>
#include <QTabBar>
#include "../Parameter.h"
#include <../util-QT/Util.h>
#include "journalineviewer.h"
#include "bwsviewerwidget.h"
#include "slideshowwidget.h"
#include "audiodetailwidget.h"
#include "EPGDlg.h"
#include "channelwidget.h"
#include <../datadecoding/DataDecoder.h>
#include "receivercontroller.h"

#define CHANNEL_POS 128
#define STREAM_POS 129
#define AFS_POS 130
#define GPS_POS 131
#define MAX_ENGINEERING_POS 192

DreamTabWidget::DreamTabWidget(ReceiverController* rc, QWidget *parent) :
    QTabWidget(parent),controller(rc),eng(false)
{
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(on_currentChanged(int)));
}

void DreamTabWidget::onServiceChanged(int short_id, const CService& service)
{
    int audioIndex = -1;
    for(int i=0; i<count(); i++)
    {
        if(tabBar()->tabData(i).toInt()==short_id)
            audioIndex = i;
    }
    int dataIndex = -1;
    for(int i=0; i<count(); i++)
    {
        if((tabBar()->tabData(i).toInt())==(4+short_id))
            dataIndex = i;
    }
    QString l;
    if(service.strLabel=="")
        l = QString("%1").arg(service.iServiceID,4,16,'0');
    else
        l = QString::fromUtf8(service.strLabel.c_str());
    if(audioIndex!=-1)
    {
        setTabText(audioIndex, l);
    }
    else
    {
       if(service.AudioParam.iStreamID!=STREAM_ID_NOT_USED)
       {
           AudioDetailWidget* pApp = new AudioDetailWidget(controller);
           pApp->setEngineering(eng);
           add(pApp, l, short_id);
       }
    }
    if(service.eAudDataFlag==CService::SF_AUDIO)
    {
        l = l + " " + GetDataTypeString(service);
    }
    if(dataIndex!=-1)
    {
        setTabText(dataIndex, l);
    }
    else
    {
        if(service.DataParam.iStreamID!=STREAM_ID_NOT_USED)
        {
           add(makeDataApp(short_id, service), l, 4+short_id);
        }
    }
}

void DreamTabWidget::on_currentChanged(int index)
{
    int short_id = tabBar()->tabData(index).toInt();
    if(short_id>=CHANNEL_POS)
        return; // it is not a service tab
    if(short_id<4)
        emit audioServiceSelected(short_id);
    else
        emit dataServiceSelected(short_id-4);
}

void DreamTabWidget::add(QWidget* w, const QString& l, int ordering)
{
    int before = count();
    for(int i=0; i<count(); i++)
    {
        if(tabBar()->tabData(i).toInt()>ordering)
        {
            before=i;
            break;
        }
    }
    int index = insertTab(before, w, l);
    tabBar()->setTabData(index, ordering);
}

QWidget* DreamTabWidget::makeDataApp(int short_id, const CService& service) const
{
    QWidget* pApp = NULL;
    if (service.DataParam.ePacketModInd == CDataParam::PM_PACKET_MODE)
        pApp = makePacketApp(short_id, service);
    if(pApp==NULL)
    {
        pApp = new QLabel(QString("short id %1 stream %2 packet id %3")
                                       .arg(short_id)
                                       .arg(service.DataParam.iStreamID)
                                       .arg(service.DataParam.iPacketID));
    }
    return pApp;
}

QWidget* DreamTabWidget::makePacketApp(int short_id, const CService& service) const
{
    QWidget *pApp = NULL;
    if (service.DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
    {
        switch (service.DataParam.iUserAppIdent)
        {
        case DAB_AT_MOTSLIDESHOW:
        {
            SlideShowWidget* p = new SlideShowWidget();
            p->setServiceInformation(short_id, service);
            connect(controller, SIGNAL(dataStatusChanged(int, ETypeRxStatus)), p, SLOT(setStatus(int, ETypeRxStatus)));
            pApp = p;
        }
            break;

        case DAB_AT_BROADCASTWEBSITE:
        {
            BWSViewerWidget* p = new BWSViewerWidget();
            p->setDecoder(service.DataParam.pDecoder);
            p->setServiceInformation(short_id, service);
            connect(controller, SIGNAL(dataStatusChanged(int, ETypeRxStatus)), p, SLOT(setStatus(int, ETypeRxStatus)));
            pApp = p;
        }
            break;

        case DAB_AT_EPG:
            break;

        case DAB_AT_JOURNALINE:
        {
            JournalineViewer* p = new JournalineViewer(short_id);
            p->setDecoder(service.DataParam.pDecoder);
            p->setServiceInformation(service, service.iServiceID);
            connect(controller, SIGNAL(dataStatusChanged(int, ETypeRxStatus)), p, SLOT(setStatus(int, ETypeRxStatus)));
            pApp = p;
        }
        break;

        default:
            ;
        }
    }
    return pApp;
}

void DreamTabWidget::setText(int short_id, const QString& text)
{
    QString t(text);
    Linkify(t);
    QTabBar* tb = tabBar();
    for(int i=0; i<tb->count(); i++)
    {
        int n = tb->tabData(i).toInt();
        if(short_id==n)
        {
            AudioDetailWidget* adw = dynamic_cast<AudioDetailWidget*>(widget(i));
            if(adw)
                adw->setTextMessage(t);
        }
    }
}

void DreamTabWidget::on_engineeringMode(bool b)
{
    eng = b;
    if(eng)
    {
        int iPlotStyle = 0;// TODO set from menu
        ChannelWidget* pCh = new ChannelWidget(controller);
        pCh->setPlotStyle(iPlotStyle);
        controller->setControls(); // new controls so fill their values from the receiver controller
        //connect(parent, SIGNAL(plotStyleChanged(int)), pCh, SLOT(setPlotStyle(int)));
        add(pCh, "Channel", CHANNEL_POS);
        add(new QLabel("Streams"), "Streams", STREAM_POS);
        add(new QLabel("AFS"), "AFS", AFS_POS);
        add(new QLabel("GPS"), "GPS", GPS_POS);
    }
    else
    {
        for(int i=tabBar()->count()-1; i>=0; --i)
        {
            int n = tabBar()->tabData(i).toInt();
            if((CHANNEL_POS<=n) && (n<=MAX_ENGINEERING_POS))
            {
                tabBar()->removeTab(i);
            }
        }
    }
    QList<AudioDetailWidget*> list = findChildren<AudioDetailWidget*>();
    for (int i = 0; i < list.size(); ++i) {
       list[i]->setEngineering(eng);
    }
}

void DreamTabWidget::removeServices()
{
    for(int i=tabBar()->count()-1; i>=0; --i)
    {
        int n = tabBar()->tabData(i).toInt();
        if(n<CHANNEL_POS)
        {
            tabBar()->removeTab(i);
        }
    }
}

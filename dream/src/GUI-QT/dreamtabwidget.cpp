#include "dreamtabwidget.h"
#include <QLabel>
#include <QVariant>
#include <QTabBar>
#include "../Parameter.h"
#include <../util-QT/Util.h>

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
                    addDataTab(short_id, service, false);
            }
            else
            {
                addDataTab(short_id, service, true);
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

void DreamTabWidget::setText(QString)
{
    // todo if curselaudioservice is visible set text in it.
}

void DreamTabWidget::addAudioTab(int short_id, const CService& service)
{
    QString l = QString::fromUtf8(service.strLabel.c_str());
    int index = addTab(new QLabel(QString("short id %1").arg(short_id)), l);
    tabBar()->setTabData(index, short_id);
}

void DreamTabWidget::addDataTab(int short_id, const CService& service, bool isDataService)
{
    QString l = QString::fromUtf8(service.strLabel.c_str());
    if(!isDataService)
    {
        l = l + " " + GetDataTypeString(service);
    }
    int index = addTab(new QLabel(QString("short id %1 stream %2 packet id %3")
                                  .arg(short_id)
                                  .arg(service.DataParam.iStreamID)
                                  .arg(service.DataParam.iPacketID)), l);
    tabBar()->setTabData(index, 4+short_id);
}

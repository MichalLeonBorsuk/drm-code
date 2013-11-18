#include "dreamtabwidget.h"
#include <QLabel>
#include <QVariant>
#include <QTabBar>
#include "../Parameter.h"

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
            QString l = QString::fromUtf8(service.strLabel.c_str());
            int index = addTab(new QLabel(QString("short id %1").arg(short_id)), l);
            tabBar()->setTabData(index, short_id);
            // TODO add tabs for data components of audio services
        }
        else
        {
            // TODO
        }
    }
}

void DreamTabWidget::on_currentChanged(int index)
{
    emit audioServiceSelected(tabBar()->tabData(index).toInt());
    // TODO data services and components
}

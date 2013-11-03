#include "audiodetailwidget.h"
#include "ui_audiodetailwidget.h"
#include <../util-QT/Util.h>
#include <../tables/TableFAC.h>

AudioDetailWidget::AudioDetailWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AudioDetailWidget)
{
    ui->setupUi(this);
}

AudioDetailWidget::~AudioDetailWidget()
{
    delete ui;
}

void AudioDetailWidget::addItem(const QString& key, const QString& val)
{
    ui->treeWidget->addTopLevelItem((new QTreeWidgetItem(QStringList() << key << val)));
}

void AudioDetailWidget::updateDisplay(int id, const CService& s)
{
    short_id = id;
    ui->treeWidget->clear();
    addItem( tr("Codec"),  GetCodecString(s));
    addItem( tr("Mode"),  GetTypeString(s));
    addItem( tr("Decodable"),  s.AudioParam.bCanDecode?tr("Yes"):tr("No"));
    addItem( tr("Text Messages"),  s.AudioParam.bTextflag?tr("Yes"):tr("No"));
    addItem( tr("Language Code"),  s.strLanguageCode.c_str());
    addItem( tr("Language"),  GetISOLanguageName(s.strLanguageCode).c_str());
    addItem( tr("Country"),  GetISOCountryName(s.strCountryCode).c_str());
    addItem( tr("Service ID"), QString("%1").arg(s.iServiceID, 6, 16));
    addItem( tr("Conditional Access"),  s.CA_USED?tr("Yes"):tr("No"));
    addItem( tr("Stream ID"),  QString("%1").arg(s.AudioParam.iStreamID));
    addItem( tr("Short ID"),  QString("%1").arg(id));

    ui->buttonListen->setEnabled(s.AudioParam.bCanDecode);
}

void AudioDetailWidget::on_buttonListen_clicked()
{
    emit listen(short_id);
}

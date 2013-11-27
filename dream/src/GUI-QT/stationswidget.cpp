#include "stationswidget.h"
#include <ui_stationswidget.h>
#include "receivercontroller.h"

StationsWidget::StationsWidget(ReceiverController* rc, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StationsWidget),
    eRecMode(RM_NONE),
    schedule(),
    strColumnParamdrm(""),strColumnParamanalog("")
{
    ui->setupUi(this);

    connect(this, SIGNAL(frequencyChanged(int)), rc, SLOT(setFrequency(int)));
    connect(rc, SIGNAL(mode(int)), this, SLOT(OnSwitchMode(int)));
    connect(rc, SIGNAL(frequencyChanged(int)), this, SLOT(SetFrequency(int)));
}

StationsWidget::~StationsWidget()
{
    delete ui;
}

void StationsWidget::OnSwitchMode(int m)
{
    ERecMode eNewRecMode = ERecMode(m);
    if(eNewRecMode != eRecMode)
    {
        CSchedule::ESchedMode eSchedM = schedule.GetSchedMode();
        bool bIsDRM = eNewRecMode == RM_DRM;
        /* Store previous columns settings */
        if (eSchedM != CSchedule::SM_NONE)
            ColumnParamToStr(bIsDRM?strColumnParamdrm:strColumnParamanalog);
        /* get sorting and filtering behaviour */
        switch (eNewRecMode)
        {
        case RM_DRM:
            schedule.SetSchedMode(CSchedule::SM_DRM);
            break;
        case RM_AM:
            schedule.SetSchedMode(CSchedule::SM_ANALOG);
            break;
        case RM_FM:
            schedule.SetSchedMode(CSchedule::SM_NONE); /* TODO */
            break;
        default: // can't happen!
            ;
        }
        /* Restore columns settings */
        ColumnParamFromStr(bIsDRM?strColumnParamdrm:strColumnParamanalog);
    }
    eRecMode = eNewRecMode;
    ui->comboBoxFilterTarget->clear();
    ui->comboBoxFilterCountry->clear();
    ui->comboBoxFilterLanguage->clear();
    ui->comboBoxFilterTarget->addItems(schedule.ListTargets);
    ui->comboBoxFilterCountry->addItems(schedule.ListCountries);
    ui->comboBoxFilterLanguage->addItems(schedule.ListLanguages);

    QString targetFilter,countryFilter,languageFilter;
    if(schedule.GetSchedMode()==CSchedule::SM_DRM)
    {
        targetFilter=schedule.targetFilterdrm;
        countryFilter=schedule.countryFilterdrm;
        languageFilter=schedule.languageFilterdrm;
    }
    else
    {
        targetFilter=schedule.targetFilteranalog;
        countryFilter=schedule.countryFilteranalog;
        languageFilter=schedule.languageFilteranalog;
    }
    ui->comboBoxFilterTarget->setCurrentIndex(ui->comboBoxFilterTarget->findText(targetFilter));
    ui->comboBoxFilterCountry->setCurrentIndex(ui->comboBoxFilterCountry->findText(countryFilter));
    ui->comboBoxFilterLanguage->setCurrentIndex(ui->comboBoxFilterLanguage->findText(languageFilter));
}

void StationsWidget::SetFrequency(int f)
{
    ui->frequency->setValue(f);
}

void StationsWidget::on_frequency_valueChanged(int)
{

}

void StationsWidget::ColumnParamFromStr(const QString& strColumnParam)
{
    QStringList list(strColumnParam.split(QChar('|')));
    const int n = list.count(); /* width and position */
    if (n == 2)
    {
        for (int j = 0; j < n; j++)
        {
            int c = ui->stations->header()->count();
            QStringList values(list[j].split(QChar(',')));
            const int lc = (int)values.count();
            if (lc < c)
                c = lc;
            for (int i = 0; i < c; i++)
            {
                int v = values[i].toInt();
                if (!j) /* width*/
                    ui->stations->header()->resizeSection(i, v);
                else /* position */
                    ui->stations->header()->moveSection(ui->stations->header()->visualIndex(i), v);
            }
        }
    }
    else
    {
        ui->stations->header()->resizeSections(QHeaderView::ResizeToContents);
        ui->stations->header()->resizeSections(QHeaderView::Interactive);
        ui->stations->header()->resizeSection(0, ui->stations->header()->minimumSectionSize());
    }
}

void StationsWidget::ColumnParamToStr(QString& strColumnParam)
{
    strColumnParam = "";
    const int n = 2; /* width and position */
    for (int j = 0; j < n; j++)
    {
        const int c = ui->stations->header()->count();
        for (int i = 0; i < c; i++)
        {
            int v;
            if (!j) /* width*/
                v = ui->stations->header()->sectionSize(i);
            else /* position */
                v = ui->stations->header()->visualIndex(i);
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

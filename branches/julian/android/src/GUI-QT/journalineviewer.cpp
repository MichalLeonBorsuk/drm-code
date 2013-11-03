#include "journalineviewer.h"
#include "ui_journalineviewer.h"
#include <../util-QT/Util.h>
#include <QFontDialog>

JournalineViewer::JournalineViewer(CDRMReceiver& rx, CSettings& st, int s, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::JournalineViewer),receiver(rx),settings(st),short_id(s)
{
    ui->setupUi(this);
    ui->textBrowser->setDocument(&document);

    //connect(actionSet_Font, SIGNAL(triggered()), SLOT(OnSetFont()));

    /* Update time for color LED */
    ui->LEDStatus->SetUpdateTime(1000);

    /* Connect controls */
    connect(ui->textBrowser, SIGNAL(backwardAvailable(bool)), ui->buttonStepBack, SLOT(setEnabled(bool)));
    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

    on_buttonClearCache_clicked();
}

JournalineViewer::~JournalineViewer()
{
    delete ui;
}

void JournalineViewer::show()
{
    /* Retrieve the font setting saved into the .ini file */
    const QString strFontFamily = settings.Get("fontfamily", string()).c_str();
    if (strFontFamily != "")
    {
        QFont fontTextBrowser = QFont(strFontFamily,
                                      settings.Get("Journaline", "fontpointsize", 0),
                                      settings.Get("Journaline", "fontweight", 0),
                                      settings.Get("Journaline", "fontitalic", false));
        ui->textBrowser->setFont(fontTextBrowser);
    }

    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();
    const int iCurSelAudioServ = Parameters.GetCurSelAudioService();
    const uint32_t iAudioServiceID = Parameters.Service[iCurSelAudioServ].iServiceID;

    /* Get current data service */
    int shortID = Parameters.GetCurSelDataService();
    CService service = Parameters.Service[shortID];
    Parameters.Unlock();

    CDataDecoder* dec = receiver.GetDataDecoder();
    if(dec)
    {
        ui->textBrowser->setDecoder(dec);
        decoderSet = true;
    }
    ui->textBrowser->setSource(QUrl("0"));

    /* Add the service description into the dialog caption */
    QString strTitle = tr("Journaline");

    if (service.IsActive())
    {
        /* Do UTF-8 to QString (UNICODE) conversion with the label strings */
        QString strLabel = QString().fromUtf8(service.strLabel.c_str()).trimmed();


        /* Service ID (plot number in hexadecimal format) */
        QString strServiceID = "";

        /* show the ID only if differ from the audio service */
        if ((service.iServiceID != 0) && (service.iServiceID != iAudioServiceID))
        {
            if (strLabel != "")
                strLabel += " ";

            strServiceID = "- ID:" +
                           QString().setNum(long(service.iServiceID), 16).toUpper();
        }

        /* add the description on the title of the dialog */
        if (strLabel != "" || strServiceID != "")
            strTitle += " [" + strLabel + strServiceID + "]";
    }
    setWindowTitle(strTitle);

    /* Update window */
    OnTimer();

    /* Activate real-time timer when window is shown */
    Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void JournalineViewer::hide()
{
    /* Deactivate real-time timer so that it does not get new pictures */
    Timer.stop();

    /* Store current textBrowser font */
    QFont fontTextBrowser = ui->textBrowser->currentFont();
    settings.Put("Journaline", "fontfamily", fontTextBrowser.family().toStdString());
    settings.Put("Journaline", "fontpointsize", fontTextBrowser.pointSize());
    settings.Put("Journaline", "fontweight", fontTextBrowser.weight());
    settings.Put("Journaline", "fontitalic", fontTextBrowser.italic());
}

void JournalineViewer::OnTimer()
{
    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();

    /* Get current data service */
    int shortID = Parameters.GetCurSelDataService();
    CService service = Parameters.Service[shortID];
    ETypeRxStatus status = Parameters.DataComponentStatus[shortID].GetStatus();
    Parameters.Unlock();

    if(!decoderSet)
    {
        CDataDecoder* dec = receiver.GetDataDecoder();
        if(dec)
        {
            ui->textBrowser->setDecoder(dec);
            decoderSet = true;
        }
    }

    SetStatus(ui->LEDStatus, status);

    if(ui->textBrowser->changed())
    {
        ui->textBrowser->reload();
    }
}

void JournalineViewer::on_buttonClearCache_clicked()
{
    //actionClear_All->setEnabled(false);
    ui->buttonStepBack->setEnabled(false);
    ui->textBrowser->clear();
    ui->textBrowser->clearHistory();
    // TODO - clear JL object cache ?
}

void JournalineViewer::OnSetFont()
{
    bool bok;

    /* Open the font dialog */
    QFont newFont = QFontDialog::getFont(&bok, ui->textBrowser->currentFont(), this);

    if (bok == true)
    {
        /* Store the current text and then reset it */
        QString strOldText = ui->textBrowser->toHtml();
        ui->textBrowser->setText("<br>");

        ui->textBrowser->setFont(newFont);

        /* Restore the text to refresh it with the new font */
        ui->textBrowser->setText(strOldText);
    }
}

void JournalineViewer::on_buttonDecode_toggled(bool checked)
{
    if(checked)
    {
        Timer.start(GUI_CONTROL_UPDATE_TIME);
        emit activated(short_id);
    }
    else
        Timer.stop();
}

void JournalineViewer::deactivate()
{
    Timer.stop();
}

/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *  Andrea Russo
 *
 * Description:
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

#include "DialogUtil.h"
#include "GeneralSettingsDlg.h"
#include <QLineEdit>
#include <QPushButton>
#include <QValidator>
#include <QMessageBox>
#include <QCheckBox>
#include <QShowEvent>
#include <QHideEvent>
#include "ThemeCustomizer.h"

/* Implementation *************************************************************/

GeneralSettingsDlg::GeneralSettingsDlg(CSettings& NSettings, QWidget* parent) :
    QDialog(parent),Settings(NSettings)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setupUi(this);

    /* Set the validators fro the receiver coordinate */
    EdtLatitudeDegrees->setValidator(new QIntValidator(0, 90, EdtLatitudeDegrees));
    EdtLongitudeDegrees->setValidator(new QIntValidator(0, 180, EdtLongitudeDegrees));

    EdtLatitudeMinutes->setValidator(new QIntValidator(0, 59, EdtLatitudeMinutes));
    EdtLongitudeMinutes->setValidator(new QIntValidator(0, 59, EdtLongitudeMinutes));

    /* Set help text for the controls */
    AddWhatsThisHelp();

    APPLY_CUSTOM_THEME();
}

GeneralSettingsDlg::~GeneralSettingsDlg()
{
}

void GeneralSettingsDlg::showEvent(QShowEvent*)
{
}

void GeneralSettingsDlg::hideEvent(QHideEvent*)
{
}

void GeneralSettingsDlg::onGPSd(const QString& s, bool checked)
{
    if(s!="")
    {
        QStringList l = s.split(":");
        LineEditGPSHost->setText(l[0]);
        LineEditGPSPort->setText(l[1]);
        CheckBoxUseGPS->setChecked(checked);
    }
    else
    {
        LineEditGPSHost->setText("");
        LineEditGPSPort->setText("");
        CheckBoxUseGPS->setChecked(false);
    }
}

void GeneralSettingsDlg::onPosition(double latitude, double longitude)
{
    QString sVal;

    /* Extract latitude values */

    if(latitude<0.0)
    {
        latitude = -latitude;
        EdtLatitudeNS->setText("S");
    }
    else
    {
        EdtLatitudeNS->setText("N");
    }

    int degrees = int(latitude);
    int minutes = int(((floor((latitude - degrees) * 1000000) / 1000000) + 0.00005) * 60.0);

    /* Extract degrees */

    /* Latitude degrees max 2 digits */
    sVal = QString("%1").arg(degrees);

    EdtLatitudeDegrees->setText(sVal);


    sVal = QString("%1").arg(minutes);

    EdtLatitudeMinutes->setText(sVal);

    /* Extract longitude values */

    if(longitude<0.0)
    {
        longitude = -longitude;
        EdtLongitudeEW->setText("W");
    }
    else if(longitude>180.0)
    {
        longitude = 360.0-longitude;
        EdtLongitudeEW->setText("E");
    }
    else
    {
        EdtLongitudeEW->setText("E");
    }

    /* Extract degrees */

    degrees = int(longitude);
    minutes = int(((floor((longitude - degrees) * 1000000) / 1000000) + 0.00005) * 60.0);

    /* Longitude degrees max 3 digits */
    sVal = QString("%1").arg(degrees);

    EdtLongitudeDegrees->setText(sVal);

    /* Extract minutes */
    sVal = QString("%1").arg(minutes);

    EdtLongitudeMinutes->setText(sVal);
}

void GeneralSettingsDlg::on_CheckBoxUseGPS_stateChanged(int s)
{
    if(s==0)
        emit useGPSd("");
}

void GeneralSettingsDlg::on_buttonOk_clicked()
{
    _BOOLEAN bOK = TRUE;
    _BOOLEAN bAllEmpty = TRUE;
    _BOOLEAN bAllCompiled = FALSE;

    /* Check the values and close the dialog */

    if (ValidInput(EdtLatitudeDegrees) == FALSE)
    {
        bOK = FALSE;
        QMessageBox::information(this, "Dream",
                                 tr("Latitude value must be in the range 0 to 90")
                                 ,QMessageBox::Ok);
    }
    else if (ValidInput(EdtLongitudeDegrees) == FALSE)
    {
        bOK = FALSE;

        QMessageBox::information(this, "Dream",
                                 tr("Longitude value must be in the range 0 to 180")
                                 ,QMessageBox::Ok);
    }
    else if (ValidInput(EdtLongitudeMinutes) == FALSE
             || ValidInput(EdtLatitudeMinutes) == FALSE)
    {
        bOK = FALSE;

        QMessageBox::information(this, "Dream",
                                 tr("Minutes value must be in the range 0 to 59")
                                 ,QMessageBox::Ok);
    }

    if (bOK == TRUE)
    {
        /* Check if all coordinates are empty */

        bAllEmpty = (EdtLongitudeDegrees->text()
                     + EdtLongitudeMinutes->text()
                     + EdtLongitudeEW->text()
                     + EdtLatitudeDegrees->text()
                     + EdtLatitudeMinutes->text()
                     + EdtLatitudeNS->text()
                    ) == "";

        /* Check if all coordinates are compiled */

        bAllCompiled = (EdtLongitudeDegrees->text() != "")
                       && (EdtLongitudeMinutes->text() != "")
                       && (EdtLongitudeEW->text() != "")
                       && (EdtLatitudeDegrees->text() != "")
                       && (EdtLatitudeMinutes->text() != "")
                       && (EdtLatitudeNS->text() != "");

        if (!bAllEmpty && !bAllCompiled)
        {
            bOK = FALSE;

            QMessageBox::information(this, "Dream",
                                     tr("Compile all fields on receiver coordinates")
                                     ,QMessageBox::Ok);
        }
    }

    if (bOK == TRUE)
    {
        /* save current settings */

        if (!bAllEmpty)
        {
            double latitude, longitude;

            latitude = EdtLatitudeDegrees->text().toDouble() + EdtLatitudeMinutes->text().toDouble()/60.0;
            if(EdtLatitudeNS->text()[0]=='S' || EdtLatitudeNS->text()[0]=='s')
                latitude = - latitude;

            longitude = EdtLongitudeDegrees->text().toDouble() + EdtLongitudeMinutes->text().toDouble()/60.0;
            if(EdtLongitudeEW->text()[0]=='W' || EdtLongitudeEW->text()[0]=='w')
                longitude = - longitude;

            emit position(latitude, longitude);
        }

        if(CheckBoxUseGPS->isChecked())
        {
            emit useGPSd(LineEditGPSHost->text()+":"+LineEditGPSPort->text());
        }
        else
            emit useGPSd("");

        accept(); /* If the values are valid close the dialog */
    }
}

void GeneralSettingsDlg::on_EdtLatitudeNS_textChanged(const QString& NewText)
{
    /* Only S or N char are accepted */

    const QString sVal = NewText.toUpper();

    if (sVal != "S" && sVal != "N" && sVal != "")
        EdtLatitudeNS->setText("");
    else
        EdtLatitudeNS->setText(sVal);
}

void GeneralSettingsDlg::on_EdtLongitudeEW_textChanged(const QString& NewText)
{
    /* Only E or W char are accepted */

    const QString sVal = NewText.toUpper();

    if (sVal != "E" && sVal != "W" && sVal != "")
        EdtLongitudeEW->setText("");
    else
        EdtLongitudeEW->setText(sVal);
}

_BOOLEAN GeneralSettingsDlg::ValidInput(const QLineEdit* le)
{
    QString sText;

    /* Use the validator for check if the value is valid */

    sText = le->text();

    if (sText == "")
        return TRUE;
    else
    {
        int iPosCursor = 0;
        return le->validator()->validate(sText,iPosCursor) == QValidator::Acceptable;
    }
}

QString GeneralSettingsDlg::ExtractDigits(const QString strStr, const int iStart
        , const int iDigits)
{
    QString sVal;
    QChar ch;
    _BOOLEAN bStop;

    /* Extract the first iDigits from position iStart */

    sVal = "";
    bStop = FALSE;

    for (int i = iStart - 1 ; i <= iStart + iDigits - 1; i++)
    {
        if (bStop == FALSE)
        {
            ch = strStr.at(i);
            if (ch.isDigit() == TRUE)
                sVal = sVal + ch;
            else
                bStop = TRUE;
        }
    }
    return sVal;
}

void GeneralSettingsDlg::AddWhatsThisHelp()
{
    QString str =
        tr("<b>Receiver coordinates:</b> Are used on "
           "Live Schedule Dialog to show a little green cube on the left"
           " of the target column if the receiver coordinates (latitude and longitude)"
           " are inside the target area of this transmission.<br>"
           "Receiver coordinates are also saved into the Log file.");
    setWhatsThis(str);
}


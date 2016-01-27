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

#ifndef __GENERAL_SETTINGS_DLG_H
#define __GENERAL_SETTINGS_DLG_H

#include "../Parameter.h"
#include "../util/Settings.h"

#include <qglobal.h>
#include <QDialog>
#include "ui_GeneralSettingsDlgbase.h"

/* Definitions ****************************************************************/

/* Classes ********************************************************************/
class GeneralSettingsDlg : public QDialog, public Ui_CGeneralSettingsDlgBase
{
    Q_OBJECT

public:
    GeneralSettingsDlg(CSettings& NSettings, QWidget* parent = 0);
    virtual ~GeneralSettingsDlg();
public slots:
    void onPosition(double latitude, double longitude);
    void onGPSd(const QString&, bool);

protected:
    void    showEvent(QShowEvent* pEvent);
    void    hideEvent(QHideEvent* pEvent);

    _BOOLEAN    ValidInput(const QLineEdit* le);
    QString     ExtractDigits(const QString strS, const int iStart, const int iDigits);

    void            AddWhatsThisHelp();
    CSettings&      Settings;

private slots:
    void on_EdtLatitudeNS_textChanged(const QString&);
    void on_EdtLongitudeEW_textChanged(const QString&);
    void on_buttonOk_clicked();
    void on_CheckBoxUseGPS_stateChanged(int);
signals:
    void useGPSd(const QString&);
    void position(double, double);

};

#endif

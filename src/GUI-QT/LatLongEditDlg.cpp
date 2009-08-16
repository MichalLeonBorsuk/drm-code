/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable, Andrea Russo
 *
 * Description:
 * edit lat or long
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

#include <iostream>
using namespace std;
#include <QLineEdit>
#include <QShowEvent>
#include <QShowEvent>
#include <QStringList>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QValidator>
#include "LatLongEditDlg.h"
#include "ReceiverSettingsDlg.h"

/* Implementation *************************************************************/
const QChar ring = '\xb0';

LatLongEditDlg::LatLongEditDlg(
	QLineEdit* s,
		QWidget* parent, const char* name, bool modal, Qt::WFlags f) :
	QDialog(parent, f), Ui_LatLongEditDlg(), d(s)
{

	/* Connections */

	connect(PushButtonOK, SIGNAL(clicked()), SLOT(ButtonOkClicked()) );
	connect(PushButtonCancel, SIGNAL(clicked()), SLOT(ButtonCancelClicked()) );

	/* Set help text for the controls */
	AddWhatsThisHelp();

}

LatLongEditDlg::~LatLongEditDlg()
{
}

void LatLongEditDlg::showEvent(QShowEvent*)
{
	if(d==NULL)
		return;
	QString t = d->text();
	QStringList a = t.split("'");
	QStringList b = a[0].split(ring);

	ComboBoxNSEW->clear();
	/* Set the validators for the receiver coordinate */
	if(a[1]=="N" || a[1]=="S")
	{
		LineEditDegrees->setValidator(new QIntValidator(0, 90, LineEditDegrees));
		ComboBoxNSEW->addItem("N");
		ComboBoxNSEW->addItem("S");
		ComboBoxNSEW->setCurrentIndex((a[1]=="N")?0:1);
	}
	else
	{
		LineEditDegrees->setValidator(new QIntValidator(0, 180, LineEditDegrees));
		ComboBoxNSEW->addItem("E");
		ComboBoxNSEW->addItem("W");
		ComboBoxNSEW->setCurrentIndex((a[1]=="E")?0:1);
	}
	LineEditMinutes->setValidator(new QIntValidator(0, 59, LineEditMinutes));
	LineEditDegrees->setText(b[0]);
	LineEditMinutes->setText(b[1]);
}

void LatLongEditDlg::ButtonOkClicked()
{
	if(d)
		d->setText(LineEditDegrees->text()+ring+LineEditMinutes->text()+"'"+ComboBoxNSEW->currentText());
	accept(); /* close the dialog */
}

void LatLongEditDlg::ButtonCancelClicked()
{
	accept(); /* close the dialog */
}

void LatLongEditDlg::AddWhatsThisHelp()
{
	const QString strGPS =
		tr("<b>Receiver coordinates:</b> Are used on "
		"the Live Schedule Dialog to show a little green cube on the left"
		" of the target column if the receiver coordinates (latitude and longitude)"
		" are inside the target area of this transmission.<br>"
		"Receiver coordinates are also saved into the Log file.");

    LineEditDegrees->setWhatsThis(strGPS);
    LineEditMinutes->setWhatsThis(strGPS);
    ComboBoxNSEW->setWhatsThis(strGPS);
}

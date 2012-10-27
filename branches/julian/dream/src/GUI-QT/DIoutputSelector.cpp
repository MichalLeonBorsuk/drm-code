/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *
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

#include "DIoutputSelector.h"
#include <QFileDialog>

DIoutputSelector::DIoutputSelector(QWidget *parent)
:QWidget(parent), Ui_DIoutputSelector(),vecIpIf(),destinations()
{
    setupUi(this);

    GetNetworkInterfaces(vecIpIf);
    for(size_t i=0; i<vecIpIf.size(); i++)
    {
	ComboBoxInterface->addItem(vecIpIf[i].name.c_str());
    }

    LineEditAddr->setInputMask("000.000.000.000;_");
    LineEditPort->setInputMask("00009;_");

    connect(PushButtonAddIP, SIGNAL(clicked()),
	    this, SLOT(OnButtonAddIP()));
    connect(PushButtonAddFile, SIGNAL(clicked()),
	    this, SLOT(OnButtonAddFile()));
    connect(PushButtonDeleteOutput, SIGNAL(clicked()),
	    this, SLOT(OnButtonDeleteOutput()));
    connect(PushButtonChooseFile, SIGNAL(clicked()),
	    this, SLOT(OnButtonChooseFile()));
    connect(ComboBoxInterface, SIGNAL(activated(int)),
	    this, SLOT(OnComboBoxInterfaceActivated(int)));
    connect(LineEditAddr, SIGNAL(textChanged(const QString&)),
	    this, SLOT(OnLineEditAddrChanged(const QString&)));
    connect(LineEditFile, SIGNAL(textChanged(const QString&)),
	    this, SLOT(OnLineEditFileChanged(const QString&)));
    connect(LineEditPort, SIGNAL(textChanged(const QString&)),
	    this, SLOT(OnLineEditPortChanged(const QString&)));
    connect(listViewOutputs, SIGNAL(clicked(const QModelIndex&)),
	    this, SLOT(OnItemClicked(const QModelIndex&)));

    listViewOutputs->setModel(&destinations);
}

DIoutputSelector::~DIoutputSelector()
{
}

void
DIoutputSelector::load(const vector<string>& MDIoutAddr)
{
    for(size_t i=0; i<MDIoutAddr.size(); i++)
    {
	QString addr = MDIoutAddr[i].c_str();
	QStringList parts = addr.split(":", QString::KeepEmptyParts);
	QList<QStandardItem*> l;
	switch(parts.count())
	{
	case 0:
	    l.push_back(new QStandardItem(parts[0]));
	    break;
	case 1:
	    l.push_back(new QStandardItem(parts[0]));
	    l.push_back(new QStandardItem(""));
	    l.push_back(new QStandardItem("any"));
	    break;
	case 2:
	    l.push_back(new QStandardItem(parts[1]));
	    l.push_back(new QStandardItem(parts[0]));
	    l.push_back(new QStandardItem("any"));
	    break;
	case 3:
	    {
		QString name;
		for(size_t j=0; j<vecIpIf.size(); j++)
		{
		    if(parts[0].toUInt()==vecIpIf[j].addr)
			name = vecIpIf[j].name.c_str();
		}
		l.push_back(new QStandardItem(parts[2]));
		l.push_back(new QStandardItem(parts[1]));
		l.push_back(new QStandardItem(name));
	    }
	}
	destinations.appendRow(l);
    }
}

void
DIoutputSelector::save(vector<string>& MDIoutAddr) const
{
	for (int i=0; i<destinations.rowCount(); i++)
	{
		QString port = destinations.item(i, 0)->text();
		QString dest = destinations.item(i, 1)->text();
		QString iface = destinations.item(i, 2)->text();
		QString addr;
		if(dest == "")
		{
		    addr = port;
		}
		else if(iface=="any")
		{
			addr = dest+":"+port;
		}
		else
		{
			addr = iface+":"+dest+":"+port;
		}
		MDIoutAddr.push_back(string(addr.toStdString()));
	}
}

void
DIoutputSelector::OnButtonAddIP()
{
    QString dest = LineEditAddr->text();
    if(dest == "...")
	    dest = "";
    QList<QStandardItem*> l;
    l.push_back(new QStandardItem(LineEditPort->text()));
    l.push_back(new QStandardItem(dest));
    l.push_back(new QStandardItem(ComboBoxInterface->currentText()));
    destinations.appendRow(l);
}

void
DIoutputSelector::OnButtonAddFile()
{
    QString file = LineEditFile->text();
    if(file != "")
    {
	QList<QStandardItem*> l;
	l.push_back(new QStandardItem(file));
	destinations.appendRow(l);
    }
}

void
DIoutputSelector::OnButtonDeleteOutput()
{
    destinations.removeRow(listViewOutputs->currentIndex().row());
}

void DIoutputSelector::OnButtonChooseFile()
{
    QString s( QFileDialog::getSaveFileName(NULL,
			"out.pcap", "Capture Files (*.pcap)" ) );
    if ( s.isEmpty() )
	return;
    LineEditFile->setText(s);
}

void DIoutputSelector::OnComboBoxInterfaceActivated(int)
{
}

void DIoutputSelector::OnLineEditAddrChanged(const QString&)
{
}

void DIoutputSelector::OnLineEditFileChanged(const QString&)
{
}

void DIoutputSelector::OnLineEditPortChanged(const QString&)
{
}

void DIoutputSelector::OnItemClicked(const QModelIndex& index)
{
    int row = index.row();
    QString dest = destinations.item(row,1)->text();
    if(dest == "")
    {
    	LineEditFile->setText(destinations.item(row,0)->text());
	LineEditPort->setText("");
	LineEditAddr->setText("");
	ComboBoxInterface->setCurrentIndex(ComboBoxInterface->findText("any"));
    }
    else
    {
    	LineEditFile->setText("");
	LineEditPort->setText(destinations.item(row,0)->text());
	LineEditAddr->setText(dest);
	int i = ComboBoxInterface->findText(destinations.item(row,2)->text());
	ComboBoxInterface->setCurrentIndex(i);
    }
}


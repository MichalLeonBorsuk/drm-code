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

#ifndef _DIOUTPUTSELECTOR_H
#define _DIOUTPUTSELECTOR_H

#include "ui_DIoutputSelector.h"
#include "../util/Utilities.h"
#include <vector>
#include <QStandardItemModel>

namespace Ui {
    class DIoutputSelector;
}

class DIoutputSelector : public QWidget, public Ui_DIoutputSelector {
    Q_OBJECT
    Q_DISABLE_COPY(DIoutputSelector)
public:
	explicit DIoutputSelector(QWidget *parent = 0);
	virtual ~DIoutputSelector();
	void	load(const vector<string>&);
	void	save(vector<string>&) const;

public slots:
	void OnButtonAddIP();
	void OnButtonAddFile();
	void OnButtonDeleteOutput();
	void OnButtonChooseFile();
	void OnComboBoxInterfaceActivated(int iID);
	void OnLineEditAddrChanged(const QString& str);
	void OnLineEditFileChanged(const QString& str);
	void OnLineEditPortChanged(const QString& str);
	void OnItemClicked(const QModelIndex&);
protected:
	vector<CIpIf>       vecIpIf;
	QStandardItemModel  destinations;
};

#endif

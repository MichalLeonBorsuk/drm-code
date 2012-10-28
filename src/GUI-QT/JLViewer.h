/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description: Journaline Viewer
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

#ifndef _JLVIEWER_H
#define _JLVIEWER_H

#include <QTextDocument>
#include "ui_JLViewer.h"
#include "../DrmReceiver.h"
#include <string>

class CSettings;

class JLViewer : public QMainWindow, Ui_JLViewer
{
	Q_OBJECT

public:
	JLViewer(CDRMReceiver&, CSettings&, QWidget* parent = 0,
		const char* name = 0, Qt::WFlags f = 0);
	virtual ~JLViewer();

protected:

    QTimer Timer;
	QTextDocument           document;
	std::string             strCurrentSavePath;
	CDRMReceiver&			receiver;
	CSettings&              settings;
	bool                    decoderSet;

public slots:
	void OnTimer();
	void OnButtonStepBack();
	void OnSave();
	void OnSaveAll();
	void OnClearAll();
	void OnSetFont();
    void showEvent(QShowEvent*);
    void hideEvent(QHideEvent*);
};

#endif

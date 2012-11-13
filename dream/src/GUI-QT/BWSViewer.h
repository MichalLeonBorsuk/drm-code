/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description: MOT Broadcast Website Viewer
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

#ifndef _BWSVIEWER_H
#define _BWSVIEWER_H

#include "ui_BWSViewer.h"
#include <string>

#include "../DrmReceiver.h"
#include "DialogUtil.h"

class CSettings;
class CDataDecoder;

class BWSViewer : public QDialog, Ui_BWSViewer
{
    Q_OBJECT

public:
    BWSViewer(CDRMReceiver&, CSettings&, QWidget* parent = 0, Qt::WFlags f = 0);
    virtual ~BWSViewer();

protected:

    QTimer Timer;
    CDRMReceiver&	receiver;
    CSettings&          settings;
    bool                decoderSet;
    bool                initialised;
    QString		strCurrentSavePath;
    bool changed();
    void CreateDirectories(const QString& filename);
    void SaveMOTObject(const CVector<_BYTE>& vecbRawData, const QString& strFileName);
    CDataDecoder*   decoder;
    std::map<QString,QVariant> pages;
    QString         sHomeUrl;
    CEventFilter    ef;

public slots:
    void OnTimer();
    void OnSave();
    void OnSaveAll();
    void OnClearAll();
    void OnHome();
    void onSetProfile(bool);
    void OnlinkClicked(const QUrl & url);
    void showEvent(QShowEvent*);
    void hideEvent(QHideEvent*);
};

#endif


/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable, David Flamand
 *
 * Description: MOT Slide Show Viewer
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

#ifndef _SLIDESHOWVIEWER_H
#define _SLIDESHOWVIEWER_H

#include "ui_SlideShowViewer.h"
#include "CWindow.h"
#include "../DrmReceiver.h"

class SlideShowViewer : public CWindow, public Ui_SlideShowViewer
{
    Q_OBJECT

public:
    SlideShowViewer(CSettings&, QWidget* parent = 0);
    virtual ~SlideShowViewer();
public slots:
    void setStatus(int, ETypeRxStatus);
    void setServiceInformation(int, CService);

protected:
    virtual void            eventShow(QShowEvent*);
    virtual void            eventHide(QHideEvent*);
    void                    SetImage(int);
    void                    UpdateButtons();
    QTimer                  Timer;
    QString                 strCurrentSavePath;
    vector<QPixmap>         vecImages;
    vector<QString>         vecImageNames;
    int                     iCurImagePos;
    bool                    bClearMOTCache;
    CService                service;
    int                     short_id;
    CDataDecoder*           decoder;

public slots:
    void OnTimer();
    void OnButtonStepBack();
    void OnButtonStepForward();
    void OnButtonJumpBegin();
    void OnButtonJumpEnd();
    void OnSave();
    void OnSaveAll();
    void OnClearAll();
};

#endif

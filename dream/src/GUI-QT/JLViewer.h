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

#include "ui_JLViewer.h"
#include "CWindow.h"
#include "../Parameter.h"
#include <QTextDocument>
#include <string>

class JLViewer : public CWindow, public Ui_JLViewer
{
    Q_OBJECT

public:
    JLViewer(CSettings&, QWidget* parent = 0);
    ~JLViewer();
public slots:
    void setSavePath(const QString&);
    void setStatus(int, ETypeRxStatus);
    void setDecoder(CDataDecoder* dec);
    void setServiceInformation(const CService&, uint32_t);

protected:
    virtual void eventShow(QShowEvent*);
    virtual void eventHide(QHideEvent*);
    QTextDocument document;
//    QString     strCurrentSavePath;

private slots:
    void on_actionSave_triggered();
    void on_actionSave_All_triggered();
    void on_actionClear_All_triggered();
    void on_actionSet_Font_triggered();
};

#endif

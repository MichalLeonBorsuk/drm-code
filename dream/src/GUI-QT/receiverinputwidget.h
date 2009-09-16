/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description:  Widget to choose sound card, rig or RCSI
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
#ifndef RECEIVERINPUTWIDGET_H
#define RECEIVERINPUTWIDGET_H

#include "ui_receiverinputwidget.h"

class CSettings;

namespace Ui {
    class ReceiverInputWidget;
}

class ReceiverInputWidget : public QWidget, public Ui_ReceiverInputWidget {
    Q_OBJECT
    Q_DISABLE_COPY(ReceiverInputWidget)
public:
    explicit ReceiverInputWidget(QWidget *parent = 0);
    virtual ~ReceiverInputWidget();

    QButtonGroup *bgrsf, *bgfriq, *bgflrm, *bgfiq, *bgsriq, *bgslrm, *bgsiq, *bgrh;
    void load(CSettings&);
    void save(CSettings&) const;

protected:
    virtual void changeEvent(QEvent *e);
};

#endif // RECEIVERINPUTWIDGET_H

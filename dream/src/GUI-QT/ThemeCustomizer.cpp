/******************************************************************************\
 * Copyright (c) 2013
 *
 * Author(s):
 *  David Flamand
 *
 * Description:
 *  Intended to provide a way to customize appearance of widget
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

#include "ThemeCustomizer.h"

#ifdef USE_THEMECUSTOMIZER

#include <QWidget>
#include <QFrame>
#include <QPalette>
#include "ui_DRMMainWindow.h"
#include "ui_serviceselector.h"

void ApplyCustomTheme(QWidget *widget, void* pUi)
{
	QString name(widget->objectName());
	if (name == "DRMMainWindow")
	{
		Ui::DRMMainWindow* ui = (Ui::DRMMainWindow*)pUi;
		QPalette palette(widget->palette());
		palette.setColor(QPalette::Window,     QColor("#000000"));
		palette.setColor(QPalette::WindowText, QColor("#FFFFFF"));
		palette.setColor(QPalette::Button,     QColor("#212421"));
		palette.setColor(QPalette::ButtonText, QColor("#FFFFFF"));
		widget->setPalette(palette);
		ui->FrameMainDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}
	else if (name == "ServiceSelector")
	{
		Ui::ServiceSelector* ui = (Ui::ServiceSelector*)pUi;
		ui->TextMiniService1->setFrameShape(QFrame::StyledPanel);
		ui->TextMiniService1->setFrameShadow(QFrame::Plain);
		ui->TextMiniService2->setFrameShape(QFrame::StyledPanel);
		ui->TextMiniService2->setFrameShadow(QFrame::Plain);
		ui->TextMiniService3->setFrameShape(QFrame::StyledPanel);
		ui->TextMiniService3->setFrameShadow(QFrame::Plain);
		ui->TextMiniService4->setFrameShape(QFrame::StyledPanel);
		ui->TextMiniService4->setFrameShadow(QFrame::Plain);
	}
}

#endif


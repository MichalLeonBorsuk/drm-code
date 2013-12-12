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

#include <QPalette>
#include <QFont>
#include <QFrame>
#include "ui_DRMMainWindow.h"
#include "ui_serviceselector.h"
#include "EvaluationDlg.h"

/*
	TODO light theme, only dark theme is currently inplemented
*/

static QPalette BasePalette(QWidget *widget)
{
	QPalette palette(widget->palette());
	palette.setColor(QPalette::Base,       QColor("#000000"));
	palette.setColor(QPalette::Text,       QColor("#FFFFFF"));
	palette.setColor(QPalette::Window,     QColor("#000000"));
	palette.setColor(QPalette::WindowText, QColor("#FFFFFF"));
	palette.setColor(QPalette::Button,     QColor("#212421"));
	palette.setColor(QPalette::ButtonText, QColor("#FFFFFF"));
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#616361"));
	widget->setPalette(palette);	
	return palette;
}

void ApplyCustomTheme(QWidget *widget, void* pUi)
{
	QString name(widget->objectName());
	if (name == "DRMMainWindow")
	{
		Ui::DRMMainWindow* ui = (Ui::DRMMainWindow*)pUi;
		QPalette palette(BasePalette(widget));
		ui->centralwidget->layout()->setMargin(0);
		ui->FrameMainDisplay->layout()->setMargin(0);
		ui->FrameMainDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		ui->TextTextMessage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
		palette = ui->TextTextMessage->palette();
		palette.setColor(QPalette::Disabled, QPalette::Light, QColor("#616361"));
		palette.setColor(QPalette::Disabled, QPalette::Dark,  QColor("#616361"));
		ui->TextTextMessage->setPalette(palette);
		QFont font(ui->ProgrInputLevel->font());
		font.setPointSize(font.pointSize()-1);
		ui->ProgrInputLevel->setFont(font);
		ui->onebar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		ui->onebar->setMinimumSize(0, 6);
		ui->onebar->setMaximumSize(16777215, 6);
		ui->twobars->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		ui->twobars->setMinimumSize(0, 6);
		ui->twobars->setMaximumSize(16777215, 12);
		ui->threebars->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		ui->threebars->setMinimumSize(0, 6);
		ui->threebars->setMaximumSize(16777215, 18);
		ui->fourbars->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		ui->fourbars->setMinimumSize(0, 6);
		ui->fourbars->setMaximumSize(16777215, 24);
		ui->fivebars->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		ui->fivebars->setMinimumSize(0, 6);
		ui->fivebars->setMaximumSize(16777215, 30);
	}
	else if (name == "ServiceSelector")
	{
		Ui::ServiceSelector* ui = (Ui::ServiceSelector*)pUi;
		QPalette palette(widget->palette());
		palette.setColor(QPalette::Light, QColor("#616361"));
		palette.setColor(QPalette::Dark,  QColor("#616361"));
		ui->TextMiniService1->setFrameShape(QFrame::Panel);
		ui->TextMiniService1->setFrameShadow(QFrame::Sunken);
		ui->TextMiniService1->setPalette(palette);
		ui->TextMiniService2->setFrameShape(QFrame::Panel);
		ui->TextMiniService2->setFrameShadow(QFrame::Sunken);
		ui->TextMiniService2->setPalette(palette);
		ui->TextMiniService3->setFrameShape(QFrame::Panel);
		ui->TextMiniService3->setFrameShadow(QFrame::Sunken);
		ui->TextMiniService3->setPalette(palette);
		ui->TextMiniService4->setFrameShape(QFrame::Panel);
		ui->TextMiniService4->setFrameShadow(QFrame::Sunken);
		ui->TextMiniService4->setPalette(palette);
		widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	}
	else if (name == "SystemEvaluationWindow")
	{
		systemevalDlg* ui = (systemevalDlg*)widget;
		QPalette palette(BasePalette(widget));
		ui->centralwidget->layout()->setMargin(0);
	}
}

#endif

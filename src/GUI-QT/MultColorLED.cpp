/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Implements a multi-color LED
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

#include "MultColorLED.h"
#include <QFrame>
#include <QLabel>


/* Implementation *************************************************************/
CMultColorLED::CMultColorLED(QWidget * parent, const char * name, Qt::WFlags f) :
	QLabel(parent, f),
	BitmCubeGreen(13, 13),BitmCubeRed(13, 13),
	BitmCubeGrey(13, 13),BitmCubeYellow(13, 13)

{

	/* Create bitmaps */
	BitmCubeGreen.fill(QColor(0, 255, 0));
	BitmCubeRed.fill(QColor(255, 0, 0));
	BitmCubeGrey.fill(QColor(192, 192, 192));
	BitmCubeYellow.fill(QColor(255, 255, 0));

	/* Set modified style */
	setFrameShape(QFrame::Panel);
	setFrameShadow(QFrame::Sunken);
	setIndent(0);

	/* Init color flags */
	Reset();

	/* Set init-bitmap */
	setPixmap(BitmCubeGrey);
	eColorFlag = RL_GREY;

	/* Init update time */
	iUpdateTime = DEFAULT_UPDATE_TIME;

	/* Connect timer events to the desired slots */
	connect(&TimerRedLight, SIGNAL(timeout()),
		this, SLOT(OnTimerRedLight()));
	connect(&TimerGreenLight, SIGNAL(timeout()),
		this, SLOT(OnTimerGreenLight()));
	connect(&TimerYellowLight, SIGNAL(timeout()),
		this, SLOT(OnTimerYellowLight()));

	TimerRedLight.setSingleShot(true);
	TimerGreenLight.setSingleShot(true);
	TimerYellowLight.setSingleShot(true);
	TimerRedLight.stop();
	TimerGreenLight.stop();
	TimerYellowLight.stop();
}

void CMultColorLED::OnTimerRedLight()
{
	bFlagRedLi = false;

	UpdateColor();
}

void CMultColorLED::OnTimerGreenLight()
{
	bFlagGreenLi = false;

	UpdateColor();
}

void CMultColorLED::OnTimerYellowLight()
{
	bFlagYellowLi = false;

	UpdateColor();
}

void CMultColorLED::Reset()
{
	/* Reset color flags */
	bFlagRedLi = false;
	bFlagGreenLi = false;
	bFlagYellowLi = false;

	UpdateColor();
}

void CMultColorLED::UpdateColor()
{
	/* Red light has highest priority, then comes yellow and at the end, we
	   decide to set green light. Allways check the current color of the
	   control before setting the color to prevent flicking */
	if (bFlagRedLi)
	{
		if (eColorFlag != RL_RED)
		{
			setPixmap(BitmCubeRed);
			eColorFlag = RL_RED;
		}
		return;
	}

	if (bFlagYellowLi)
	{
		if (eColorFlag != RL_YELLOW)
		{
			setPixmap(BitmCubeYellow);
			eColorFlag = RL_YELLOW;
		}
		return;
	}

	if (bFlagGreenLi)
	{
		if (eColorFlag != RL_GREEN)
		{
			setPixmap(BitmCubeGreen);
			eColorFlag = RL_GREEN;
		}
		return;
	}

	/* If no color is active, set control to grey light */
	if (eColorFlag != RL_GREY)
	{
		setPixmap(BitmCubeGrey);
		eColorFlag = RL_GREY;
	}
}

void CMultColorLED::SetLight(int iNewStatus)
{
	switch (iNewStatus)
	{
	case RL_GREEN:
		/* Green light */
		bFlagGreenLi = true;
		TimerGreenLight.start(iUpdateTime);
		break;

	case RL_YELLOW:
		/* Yellow light */
		bFlagYellowLi = true;
		TimerYellowLight.start(iUpdateTime);
		break;

	case RL_RED:
		/* Red light */
		bFlagRedLi = true;
		TimerRedLight.start(iUpdateTime);
		break;
	}

	UpdateColor();
}

void CMultColorLED::SetUpdateTime(int iNUTi)
{
	/* Avoid too short intervals */
	if (iNUTi < MIN_TIME_FOR_RED_LIGHT)
		iUpdateTime = MIN_TIME_FOR_RED_LIGHT;
	else
		iUpdateTime = iNUTi;
}

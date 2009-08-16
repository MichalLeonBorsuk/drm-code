/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *
 * SetLight():
 *	0: Green
 *	1: Yellow
 *	2: Red
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

#if !defined(AFX_MULTCOLORLED_H__FD6B49B5_87DF_48DD_A873_804E1606C2AC__INCLUDED_)
#define AFX_MULTCOLORLED_H__FD6B49B5_87DF_48DD_A873_804E1606C2AC__INCLUDED_

#include <QLabel>
#include <QPixmap>
#include <QTimer>


/* Definitions ****************************************************************/
#define DEFAULT_UPDATE_TIME				600

/* The red and yellow light should be on at least this interval */
#define MIN_TIME_FOR_RED_LIGHT			100


/* Classes ********************************************************************/
class CMultColorLED : public QLabel
{
    Q_OBJECT

public:
	enum ELightColor {RL_GREY, RL_RED, RL_GREEN, RL_YELLOW};
	CMultColorLED(QWidget* parent, const char * name = 0, Qt::WFlags f = 0);
	virtual ~CMultColorLED() {}

	void SetUpdateTime(int iNUTi);
	void SetLight(int iNewStatus);
	void Reset();


protected:

	QPixmap			BitmCubeGreen;
	QPixmap			BitmCubeYellow;
	QPixmap			BitmCubeRed;
	QPixmap			BitmCubeGrey;

	ELightColor		eColorFlag;

	QTimer			TimerRedLight;
	QTimer			TimerGreenLight;
	QTimer			TimerYellowLight;

	bool			bFlagRedLi;
	bool			bFlagGreenLi;
	bool			bFlagYellowLi;

	int				iUpdateTime;

	void			UpdateColor();

protected slots:
	void OnTimerRedLight();
	void OnTimerGreenLight();
	void OnTimerYellowLight();
};


#endif // AFX_MULTCOLORLED_H__FD6B49B5_87DF_48DD_A873_804E1606C2AC__INCLUDED_

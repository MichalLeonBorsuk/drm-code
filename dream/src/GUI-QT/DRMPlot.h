/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Original Author(s):
 *	Volker Fischer
 *
 * Qwt 5-6 conversion Author(s):
 *  David Flamand
 *
 * Description:
 *	Custom settings of the qwt-plot, Support Qwt version 5.0.0 to 6.1.0(+)
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

#ifndef __DRMPLOT_H
#define __DRMPLOT_H

#include <../Parameter.h>
#include <QColor>

class QTreeWidget;
class QIcon;
class QRect;
class ReceiverController;

class CDRMPlot
{

public:
    enum ECharType
    {
        INPUT_SIG_PSD = 0, /* default */
        TRANSFERFUNCTION = 1,
        FAC_CONSTELLATION = 2,
        SDC_CONSTELLATION = 3,
        MSC_CONSTELLATION = 4,
        POWER_SPEC_DENSITY = 5,
        INPUTSPECTRUM_NO_AV = 6,
        AUDIO_SPECTRUM = 7,
        FREQ_SAM_OFFS_HIST = 8,
        DOPPLER_DELAY_HIST = 9,
        ALL_CONSTELLATION = 10,
        SNR_AUDIO_HIST = 11,
        AVERAGED_IR = 12,
        SNR_SPECTRUM = 13,
        INPUT_SIG_PSD_ANALOG = 14,
        INP_SPEC_WATERF = 15,
        NONE_OLD = 16 /* None must always be the last element! (see settings) */
    };

    CDRMPlot():CurCharType(NONE_OLD){}
    virtual ~CDRMPlot() {}

    virtual void setCaption(const QString& s)=0;
    virtual void setIcon(const QIcon& s)=0;
    virtual void setGeometry(const QRect& g)=0;
    virtual bool isVisible() const = 0;
    virtual const QRect geometry() const = 0;
    virtual void show()=0;
    virtual void close()=0;
    virtual void activate()=0;
    virtual void deactivate()=0;

    virtual void SetupChart(const ECharType eNewType);
    void setupTreeWidget(QTreeWidget* tw);
    void SetPlotStyle(const int iNewStyleID);
    void update(ReceiverController* rc);
    ECharType GetChartType() const { return CurCharType; }

protected:
    virtual void applyColors()=0;
    virtual void replot()=0;
    virtual void clearPlots()=0;
    virtual void setupBasicPlot(const char* titleText,
                        const char* xText, const char* yText, const char* legendText,
                        double left, double right, double bottom, double top)=0;
    virtual void add2ndGraph(const char* axisText, const char* legendText, double bottom, double top)=0;
    virtual void addxMarker(QColor color, double initialPos)=0;
    virtual void addyMarker(QColor color, double initialPos)=0;
    virtual void setupConstPlot(const char* text, ECodScheme eNewCoSc)=0;
    virtual void addConstellation(const char* legendText, int n)=0;
    virtual void setupWaterfall()=0;
    virtual void setQAMGrid(const ECodScheme eCoSc)=0;
    virtual void setData(int n, CVector<_COMPLEX>& veccData)=0;
    virtual void setData(int n, CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, bool autoScale=false, const QString& axisLabel="")=0;
    virtual void setxMarker(int n, _REAL r)=0;
    virtual void setxMarker(int n, _REAL c, _REAL b)=0;
    virtual void setyMarker(int n, _REAL r)=0;
    virtual void updateWaterfall(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)=0;

    ECharType		CurCharType;
    QColor			MainPenColorPlot;
    QColor			MainPenColorConst;
    QColor			MainGridColorPlot;
    QColor			SpecLine1ColorPlot;
    QColor			SpecLine2ColorPlot;
    QColor			PassBandColorPlot;
    QColor			BckgrdColorPlot;

};


#endif

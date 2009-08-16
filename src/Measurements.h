/*****************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy, Julian Cable
 *
 * Description:
 *	See Measurements.cpp
 *
 *******************************************************************************
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
\*******************************************************************************/

#ifndef _MEASUREMENTS_H
#define _MEASUREMENTS_H

#include "GlobalDefinitions.h"
#include <vector>
#include <deque>
#include <limits>

class CMeasure
{
public:
    CMeasure():validdata(false),subscriptions(0) {}
    virtual ~CMeasure() {}
    void subscribe() { subscriptions++; }
    void unsubscribe() { --subscriptions; if(subscriptions<0) subscriptions=0; }
    bool wanted() const { return subscriptions>0; }
    virtual bool valid() const { return validdata; }
    virtual void invalidate() { validdata=false; }
protected:
    bool validdata;
    int subscriptions;
};

template<typename T>
class CPointMeasure : public CMeasure
{
public:
    CPointMeasure():CMeasure(),value(){}
    virtual ~CPointMeasure(){}
    void set(const T& v) { value = v; validdata=true; }
    bool get(T& v) const
    {
        if(valid())
        {
            v=value;
            return true;
        }
        return false;
    }
protected:
    T value;
};

template<typename T>
class CTimeSeries : public CMeasure
{
public:
    CTimeSeries():CMeasure(),history(),step(numeric_limits<T>::min()),
    max(numeric_limits<size_t>::max()){}
    void configure(size_t _max, _REAL _step)
    {
        max=_max; step=_step;
    }
    void invalidate() { CMeasure::invalidate(); history.clear();}
    _REAL interval() { return step; }
    void set(const T& v)
    {
        history.push_back(v);
        if(history.size()>max)
            history.pop_front();
        validdata=true;
    }
    bool get(T& v) const
    {
        if(valid())
        {
            v=history.back();
            return true;
        }
        return false;
    }
    bool get(vector<T>& v)
    {
        if(valid())
        {
            v.resize(history.size());
            v.assign(history.begin(),history.end());
            return true;
        }
        return false;
    }
protected:
    deque<T> history;
    _REAL step;
    size_t max;
};

template<typename T>
class CMinMaxMean : public CDumpable, public CMeasure
{
public:
    CMinMaxMean();

    void addSample(T);
    T getCurrent() const;
    T getMean();
    bool getCurrent(T&) const;
    bool getMean(T&);
    bool getMinMax(T&, T&);
    virtual void invalidate();
    void dump(ostream&) const;
protected:
    T sum, cur, min, max;
    int num;
};

class CMeasurements
{
public:

    CMeasurements();

	CMinMaxMean<_REAL> SNRstat, SigStrstat;

	CPointMeasure<_REAL> MER;
	CPointMeasure<_REAL> WMERMSC;
	CPointMeasure<_REAL> WMERFAC;

    /* Doppler */
    CTimeSeries<_REAL> Doppler; // Dream calculation
	CPointMeasure<_REAL> Rdop;    // RSCI calculation

    CTimeSeries<_REAL> Delay; // Dream calculation
    struct CRdel { _REAL threshold, interval; };
	CPointMeasure<vector<CRdel> > Rdel; // RSCI calculation

	/* interference (constellation-based measurement rnic)*/
	struct CInterferer { _REAL rIntFreq, rINR, rICR; };
	CPointMeasure<CInterferer> interference;

	/* peak of PSD - for PSD-based interference measurement rnip */
	_REAL rMaxPSDwrtSig;
	_REAL rMaxPSDFreq;
	bool bETSIPSD; // ETSI PSD scale or old Dream ?

	CPointMeasure<vector<_REAL> > PSD;

	CPointMeasure<_REAL> SigStr;
	CPointMeasure<_REAL> IFSigStr;

    CPointMeasure<vector<_COMPLEX> > ChannelEstimate;

	CTimeSeries<bool> audioFrameStatus;

    struct CPIR
    {
        vector <_REAL> data;
        _REAL rStart, rStep;
        _REAL rStartGuard, rEndGuard, rLowerBound, rHigherBound, rPDSBegin, rPDSEnd;
    };

    CPointMeasure<CPIR> PIR;

    CPointMeasure<_REAL> AnalogCurMixFreqOffs;
    CPointMeasure<_REAL> AnalogBW;
    CPointMeasure<_REAL> AnalogCenterFreq;

    CPointMeasure<vector<_COMPLEX> > FACVectorSpace;
    CPointMeasure<vector<_COMPLEX> > SDCVectorSpace;
    CPointMeasure<vector<_COMPLEX> > MSCVectorSpace;
    CPointMeasure<vector<_REAL> > PowerDensitySpectrum;
    CPointMeasure<vector<_REAL> > InputSpectrum;
    CPointMeasure<vector<_REAL> > AudioSpectrum;
	CPointMeasure<vector<vector<_COMPLEX> > > Pilots;

	CTimeSeries<_REAL>	FrequencySyncValue;
	CTimeSeries<_REAL>	SampleFrequencyOffset;
	CTimeSeries<_REAL>	SNRHist;
    CPointMeasure<vector<_REAL> > SNRProfile;
	CTimeSeries<int>	CDAudHist;

protected:

};

template<typename T>
CMinMaxMean<T>::CMinMaxMean():CDumpable(),CMeasure(),sum(0),cur(),
min(numeric_limits<T>::max()),max(numeric_limits<T>::min()),num(0)
{
}

template<typename T>
void CMinMaxMean<T>::invalidate()
{
    CMeasure::invalidate();
    sum = 0;
	num = 0;
	min = numeric_limits<T>::max();
	max = numeric_limits<T>::min();
}

template<typename T>
void CMinMaxMean<T>::addSample(T val)
{
	cur = val;
	sum += val;
	num++;
	if(val>max)
		max = val;
	if(val<min)
		min = val;
    validdata=true;
}

template<typename T>
T CMinMaxMean<T>::getCurrent() const
{
	return cur;
}

template<typename T>
bool CMinMaxMean<T>::getCurrent(T& val) const
{
    if(validdata)
        val = cur;
    return validdata;
}

template<typename T>
T CMinMaxMean<T>::getMean()
{
    if(!valid())
        return 0;
	T mean = 0;
	if(num>0)
		mean = sum / T(num);
    invalidate();
	return mean;
}

template<typename T>
bool CMinMaxMean<T>::getMean(T& val)
{
    if(valid())
    {
        val=getMean();
        return true;
    }
    return false;
}

template<typename T>
bool CMinMaxMean<T>::getMinMax(T& minOut, T& maxOut)
{
    if(!valid())
        return false;
    minOut = min;
    maxOut = max;
    invalidate();
    return true;
}

template<typename T>
void
CMinMaxMean<T>::dump(ostream& out) const
{
    out << "{ Sum: " <<  sum << "," << endl;
    out << "Cur: " << cur << "," << endl;
    out << "Min: " << min << "," << endl;
    out << "Max " << max << "," << endl;
    out << "Num: " << num << "}" << endl;
}

#endif

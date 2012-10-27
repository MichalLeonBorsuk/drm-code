/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	DRM-Modulator
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

#include "DrmModulator.h"
#include "util/Settings.h"
#include "sound.h"
#include <iostream>

/* Implementation *************************************************************/
CDRMModulator::CDRMModulator():
	MLCEncBuf(), CarMapBuf(), OFDMModBuf(),
	IntlBuf(), FACMapBuf(), SDCMapBuf(),
	TransmitData(),
	MSCMLCEncoder(CT_MSC), FACMLCEncoder(CT_FAC), SDCMLCEncoder(CT_SDC),
	SymbInterleaver(), OFDMCellMapping(), OFDMModulation()
{
}

void
CDRMModulator::GetSoundOutChoices(vector<string>& v) const
{
    CSoundOut s;
	s.Enumerate(v);
}

void
CDRMModulator::SetOutputs(const vector<string>& o)
{
	if(o.size()>0)
	{
		TransmitData.SetOutputs(o);
	}
	else
		throw CGenErr("Modulator with no outputs");
}

void
CDRMModulator::GetOutputs(vector<string>& o) const
{
	TransmitData.GetOutputs(o);
}

void
CDRMModulator::Init(CParameter& Parameters)
{
	cerr << "Modulator initialised with"
		<< " MSC mode " << int(Parameters.Channel.eMSCmode)
		<< " SDC mode " << int(Parameters.Channel.eSDCmode)
		<< " robm " << int(Parameters.Channel.eRobustness)
		<< " spectrum occupancy " << int(Parameters.Channel.eSpectrumOccupancy) << endl;

	/* Defines number of cells, important! */
	OFDMCellMapping.Init(Parameters, CarMapBuf);

	/* Defines number of SDC bits per super-frame */
	SDCMLCEncoder.Init(Parameters, SDCMapBuf);
	MSCMLCEncoder.Init(Parameters, MLCEncBuf);
	SymbInterleaver.Init(Parameters, IntlBuf);
	FACMLCEncoder.Init(Parameters, FACMapBuf);
	OFDMModulation.Init(Parameters, OFDMModBuf);
	TransmitData.Init(Parameters);
}

void
CDRMModulator::WriteData(CParameter& Parameters,
				CBuffer<_BINARY>& FACBuf, CBuffer<_BINARY>& SDCBuf,
				vector<CSingleBuffer<_BINARY> >& MSCBuf)
{
#if 0
	cerr << "Fill: " << FACBuf.GetFillLevel() << " " << SDCBuf.GetFillLevel() << " " << MSCBuf[0].GetFillLevel() << endl;
	cerr << "MSC MLC Int SDC FAC Car FDM" << endl;
	cerr <<  " "  << MSCBuf[0].GetRequestFlag();
	cerr << "   " << MLCEncBuf.GetRequestFlag() ;
	cerr << "   " << IntlBuf.GetRequestFlag() ;
	cerr << "   " << SDCBuf.GetRequestFlag() ;
	cerr << "   " << FACBuf.GetRequestFlag() ;
	cerr << "   " << CarMapBuf.GetRequestFlag();
	cerr << "   " << OFDMModBuf.GetRequestFlag();
	cerr << endl;
#endif
	/* MLC-encoder */
	MSCMLCEncoder.ProcessData(Parameters, MSCBuf[0], MLCEncBuf);

	/* Convolutional interleaver */
	SymbInterleaver.ProcessData(Parameters, MLCEncBuf, IntlBuf);

	/* FAC *************************************************************** */
	FACMLCEncoder.ProcessData(Parameters, FACBuf, FACMapBuf);

	/* SDC *************************************************************** */
	SDCMLCEncoder.ProcessData(Parameters, SDCBuf, SDCMapBuf);

	/* Mapping of the MSC, FAC, SDC and pilots on the carriers *********** */
	OFDMCellMapping.ProcessData(Parameters, IntlBuf, FACMapBuf, SDCMapBuf, CarMapBuf);

	/* OFDM-modulation *************************************************** */
	OFDMModulation.ProcessData(Parameters, CarMapBuf, OFDMModBuf);

	/* Transmit the signal *********************************************** */
	TransmitData.WriteData(Parameters, OFDMModBuf);
}

void
CDRMModulator::Cleanup(CParameter&)
{
}

void
CDRMModulator::LoadSettings(CSettings& s, CParameter& Parameters)
{

	/* Set desired intermediate frequency (IF) in Hertz */
	Parameters.rCarOffset = s.Get("Modulator", "if", 12000.0);

	/* default output format - REAL */
	Parameters.eOutputFormat = EOutFormat(s.Get("Modulator", "output_format", OF_REAL_VAL));

	string outputs = s.Get("Modulator", "cofdm_outputs", string(""));
    vector<string> o;
	if(outputs != "")
	{
        stringstream ss(outputs);
        string op;
        while(getline(ss, op, ','))
        {
            o.push_back(op);
        }
	}
	else
	{
	    // set default to first sound card device
        CSoundOut s;
        s.Enumerate(o);
        if(o.size()>0)
        {
            o.resize(1);
        }
        else
        {
            // set output to a file
            o.push_back("DreamCOFDMOut.wav");
        }
	}
	TransmitData.SetOutputs(o);
}

void
CDRMModulator::SaveSettings(CSettings& s, const CParameter& Parameters) const
{
	s.Put("Modulator", "if", Parameters.rCarOffset);
	s.Put("Modulator", "output_format", Parameters.eOutputFormat);
	stringstream outputs;
	string sep="";
	vector<string> o;
	TransmitData.GetOutputs(o);
	for(size_t i=0; i<o.size(); i++)
	{
		outputs << sep << o[i];
		sep = ",";
	}
	s.Put("Modulator", "cofdm_outputs", outputs.str());
}

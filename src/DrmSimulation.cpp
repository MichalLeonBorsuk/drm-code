/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM-simulation
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

#include "DrmSimulation.h"


/* Implementation *************************************************************/

CDRMSimulation::CDRMSimulation() : iSimTime(0), iSimNumErrors(0),
	rStartSNR(0.0), rEndSNR(0.0), rStepSNR(0.0),
	Param(),
	DataBuf(), MLCEncBuf(), MSC_FAC_SDC_MapBuf(),
	//IntlBuf(), FACMapBuf(), SDCMapBuf(),
	GenFACDataBuf(), GenSDCDataBuf(),
	CarMapBuf(), OFDMModBuf(), OFDMDemodBufChan2(), ChanEstInBufSim(),
	ChanEstOutBufChan(),
	RecDataBuf(), ChanResInBuf(), InpResBuf(), FreqSyncAcqBuf(), TimeSyncBuf(),
	OFDMDemodBuf(), SyncUsingPilBuf(), ChanEstBuf(),
	MSCCarDemapBuf(), FACCarDemapBuf(), SDCCarDemapBuf(), DeintlBuf(),
	FACDecBuf(), SDCDecBuf(), MSCMLCDecBuf(), GenSimData(),
	MSCMLCEncoder(CT_MSC), SymbInterleaver(), GenerateFACData(), FACMLCEncoder(CT_FAC),
	GenerateSDCData(), SDCMLCEncoder(CT_SDC), OFDMCellMapping(), OFDMModulation(),
	DRMChannel(), InputResample(), FreqSyncAcq(), TimeSync(), OFDMDemodulation(),
	SyncUsingPil(), ChannelEstimation(), OFDMCellDemapping(),
	FACMLCDecoder(CT_FAC), UtilizeFACData(),
	SDCMLCDecoder(CT_SDC), UtilizeSDCData(), SymbDeinterleaver(), MSCMLCDecoder(CT_MSC),
	EvaSimData(), OFDMDemodSimulation(), IdealChanEst(), DataConvChanResam()
{
	/* Set all parameters to meaningful value for startup state. If we want to
	   make a simulation we just have to specify the important values */
	/* Init streams */
	Param.ResetServicesStreams();

	/* Service parameters (only use service 0) ------------------------------- */
	/* Data service */
	Param.FACParameters.iNumAudioServices =0;
	Param.FACParameters.iNumDataServices = 1;

	CDataParam DataParam;
	DataParam.ePacketModInd = PM_SYNCHRON_STR_MODE;
	Param.DataParam[0][0] = DataParam;

    Param.Service[0].eAudDataFlag = SF_DATA;
	Param.Service[0].iDataStream = 0;

	//Param.SetCurSelDataService(1); /* Service ID must be set for activation */
	Param.SetCurSelDataService(0); /* Service ID must be set for activation */

	/* Stream */
    Param.MSCParameters.Stream[0].iLenPartA = 0;
    Param.MSCParameters.Stream[0].iLenPartB = 0; // EEP, if "= 0"


	/* Date, time */
	Param.iDay = 0;
	Param.iMonth = 0;
	Param.iYear = 0;
	Param.iUTCHour = 0;
	Param.iUTCMin = 0;

	/* Initialize synchronization parameters */
	Param.rResampleOffset = (_REAL) 0.0;
	Param.rFreqOffsetAcqui = (_REAL) VIRTUAL_INTERMED_FREQ / SOUNDCRD_SAMPLE_RATE;
	Param.rFreqOffsetTrack = (_REAL) 0.0;
	Param.iTimingOffsTrack = 0;

	Param.FACParameters.iFrameId = 0;

	Param.Channel.eRobustness = RM_ROBUSTNESS_MODE_B;
	Param.Channel.eSpectrumOccupancy = SO_3;
	Param.Channel.eInterleaverDepth = SI_SHORT;
	Param.Channel.eMSCmode = CS_3_SM;
	Param.Channel.eSDCmode = CS_2_SM;

	Param.MSCParameters.ProtectionLevel.iPartA = 1;
	Param.MSCParameters.ProtectionLevel.iPartB = 1;
	Param.MSCParameters.ProtectionLevel.iHierarch = 0;

	/* DRM channel parameters */
	Param.iDRMChannelNum = 1;
	Param.SetNominalSNRdB(25);
}

void CDRMSimulation::Run()
{
/*
 The hand over of data is done via an intermediate-buffer. The calling
 convention is always "input-buffer, output-buffer". Additional, the
 DRM-parameters are fed to the function
*/

	/* Initialization of the modules */
	Init();

	/* Set run flag */
	MSC_FAC_SDC_MapBuf[0] = &IntlBuf;
	MSC_FAC_SDC_MapBuf[1] = &FACMapBuf;
	MSC_FAC_SDC_MapBuf[2] = &SDCMapBuf;

	while (true) // we will be thrown out of this at the end of the run
	{
		/**********************************************************************\
		* Transmitter														   *
		\**********************************************************************/
		/* MSC -------------------------------------------------------------- */
		/* Read the source signal */
		GenSimData.ReadData(Param, DataBuf);

		/* MLC-encoder */
		MSCMLCEncoder.ProcessData(Param, DataBuf, MLCEncBuf);

		/* Convolutional interleaver */
		SymbInterleaver.ProcessData(Param, MLCEncBuf, IntlBuf);


		/* FAC -------------------------------------------------------------- */
		GenerateFACData.ReadData(Param, GenFACDataBuf);
		FACMLCEncoder.ProcessData(Param, GenFACDataBuf, FACMapBuf);


		/* SDC -------------------------------------------------------------- */
		GenerateSDCData.ReadData(Param, GenSDCDataBuf);
		SDCMLCEncoder.ProcessData(Param, GenSDCDataBuf, SDCMapBuf);


		/* Mapping of the MSC, FAC, SDC and pilots on the carriers */
		OFDMCellMapping.ProcessData(Param, IntlBuf, FACMapBuf, SDCMapBuf, CarMapBuf);

		/* OFDM-modulation */
		OFDMModulation.ProcessData(Param, CarMapBuf, OFDMModBuf);



		/**********************************************************************\
		* Channel    														   *
		\**********************************************************************/
		DRMChannel.TransferData(Param, OFDMModBuf, RecDataBuf);



		/**********************************************************************\
		* Receiver    														   *
		\**********************************************************************/
switch (Param.eSimType)
{
case CParameter::ST_MSECHANEST:
case CParameter::ST_BER_IDEALCHAN:
case CParameter::ST_SINR:
		/* MSE of channel estimation, ideal channel estimation -------------- */
		/* Special OFDM demodulation for channel estimation tests (with guard-
		   interval removal) */
		OFDMDemodSimulation.ProcessDataOut(Param, RecDataBuf,
			ChanEstInBufSim, OFDMDemodBufChan2);

		/* Channel estimation and equalization */
		ChannelEstimation.ProcessData(Param, ChanEstInBufSim, ChanEstBuf);

		/* Ideal channel estimation (with MSE calculation) */
		IdealChanEst.ProcessDataIn(Param, ChanEstBuf, OFDMDemodBufChan2,
			ChanEstBuf);
	break;


default: /* Other types like ST_BITERROR or ST_SYNC_PARAM */
		/* Bit error rate (we can use all synchronization units here!) ------ */
		/* This module converts the "CChanSimDataMod" data type of "DRMChannel"
		   to the "_REAL" data type, because a regular module can only have ONE
		   type of input buffers */
		DataConvChanResam.ProcessData(Param, RecDataBuf, ChanResInBuf);

		/* Resample input DRM-stream */
		InputResample.ProcessData(Param, ChanResInBuf, InpResBuf);

		/* Frequency synchronization acquisition */
		FreqSyncAcq.ProcessData(Param, InpResBuf, FreqSyncAcqBuf);

		/* Time synchronization */
		TimeSync.ProcessData(Param, FreqSyncAcqBuf, TimeSyncBuf);

		/* OFDM-demodulation */
		OFDMDemodulation.ProcessData(Param, TimeSyncBuf, OFDMDemodBuf);

		/* Synchronization in the frequency domain (using pilots) */
		SyncUsingPil.ProcessData(Param, OFDMDemodBuf, SyncUsingPilBuf);

		/* Channel estimation and equalization */
		ChannelEstimation.ProcessData(Param, SyncUsingPilBuf, ChanEstBuf);
	break;
}

		/* Demapping of the MSC, FAC, SDC and pilots from the carriers */
		OFDMCellDemapping.ProcessData(Param, ChanEstBuf,
			MSCCarDemapBuf, FACCarDemapBuf, SDCCarDemapBuf);

		/* FAC */
		FACMLCDecoder.ProcessData(Param, FACCarDemapBuf, FACDecBuf);
		UtilizeFACData.WriteData(Param, FACDecBuf);


		/* SDC */
		SDCMLCDecoder.ProcessData(Param, SDCCarDemapBuf, SDCDecBuf);
		UtilizeSDCData.WriteData(Param, SDCDecBuf);


		/* MSC */
		/* Symbol de-interleaver */
		SymbDeinterleaver.ProcessData(Param, MSCCarDemapBuf, DeintlBuf);

		/* MLC-decoder */
		MSCMLCDecoder.ProcessData(Param, DeintlBuf, MSCMLCDecBuf);

		/* Evaluate simulation data */
		EvaSimData.WriteData(Param, MSCMLCDecBuf);
	}
}

void CDRMSimulation::Init()
{
	/* Defines number of cells, important! */
	OFDMCellMapping.Init(Param, CarMapBuf);

	/* Defines number of SDC bits per super-frame */
	SDCMLCEncoder.Init(Param, SDCMapBuf);

	MSCMLCEncoder.Init(Param, MLCEncBuf);
	SymbInterleaver.Init(Param, IntlBuf);
	GenerateFACData.Init(Param, GenFACDataBuf);
	FACMLCEncoder.Init(Param, FACMapBuf);
	GenerateSDCData.Init(Param, GenSDCDataBuf);
	OFDMModulation.Init(Param, OFDMModBuf);
	GenSimData.Init(Param, DataBuf);

	/* Receiver modules */
	/* The order of modules are important! */
	InputResample.Init(Param, InpResBuf);
	FreqSyncAcq.Init(Param, FreqSyncAcqBuf);
	TimeSync.Init(Param, TimeSyncBuf);
	SyncUsingPil.Init(Param, SyncUsingPilBuf);

	/* Channel estimation init must be called before OFDMDemodSimulation
	   module, because the delay is set here which the other modules use! */
	ChannelEstimation.Init(Param, ChanEstBuf);

	OFDMCellDemapping.Init(Param, MSCCarDemapBuf, FACCarDemapBuf,
		SDCCarDemapBuf);
	FACMLCDecoder.Init(Param, FACDecBuf);
	UtilizeFACData.Init(Param);
	SDCMLCDecoder.Init(Param, SDCDecBuf);
	UtilizeSDCData.Init(Param);
	SymbDeinterleaver.Init(Param, DeintlBuf);
	MSCMLCDecoder.Init(Param, MSCMLCDecBuf);


	/* Special module for simulation */
	EvaSimData.Init(Param);

	/* Init channel. The channel must be initialized before the modules
	   "OFDMDemodSimulation" and "IdealChanEst" because they need iNumTaps and
	   tap delays in global struct */
	DRMChannel.Init(Param, RecDataBuf);

	/* Mode dependent initializations */
	switch (Param.eSimType)
	{
	case CParameter::ST_MSECHANEST:
	case CParameter::ST_BER_IDEALCHAN:
	case CParameter::ST_SINR:
		/* Init OFDM demod before IdealChanEst, because the timing offset of
		   useful part extraction is set here */
		OFDMDemodSimulation.Init(Param, ChanEstInBufSim, OFDMDemodBufChan2);

		/* Problem: "ChanEstBuf" is used for input and output buffer. That only
		   works with single buffers. This solution works for this case but is
		   not a very nice solution FIXME */
		IdealChanEst.Init(Param, ChanEstBuf);
		break;

	default: /* Other types like ST_BITERROR or ST_SYNC_PARAM */
		DataConvChanResam.Init(Param, ChanResInBuf);

		OFDMDemodulation.Init(Param, OFDMDemodBuf);
		break;
	}


	/* Clear all buffers ---------------------------------------------------- */
	/* The buffers must be cleared in case there is some data left in the
	   buffers from the last simulation (with, e.g., different SNR) */
	DataBuf.Clear();
	MLCEncBuf.Clear();
	IntlBuf.Clear();
	FACMapBuf.Clear();
	SDCMapBuf.Clear();
	GenFACDataBuf.Clear();
	GenSDCDataBuf.Clear();
	CarMapBuf.Clear();
	OFDMModBuf.Clear();
	OFDMDemodBufChan2.Clear();
	ChanEstInBufSim.Clear();
	ChanEstOutBufChan.Clear();
	RecDataBuf.Clear();
	ChanResInBuf.Clear();
	InpResBuf.Clear();
	FreqSyncAcqBuf.Clear();
	TimeSyncBuf.Clear();
	OFDMDemodBuf.Clear();
	SyncUsingPilBuf.Clear();
	ChanEstBuf.Clear();
	MSCCarDemapBuf.Clear();
	FACCarDemapBuf.Clear();
	SDCCarDemapBuf.Clear();
	DeintlBuf.Clear();
	FACDecBuf.Clear();
	SDCDecBuf.Clear();
	MSCMLCDecBuf.Clear();


	/* We only want to simulate tracking performance ------------------------ */
	TimeSync.StopTimingAcqu();
	TimeSync.StopRMDetAcqu(); /* Robustness mode detection */
	ChannelEstimation.GetTimeSyncTrack()->StartTracking();

	/* We stop tracking of time wiener interpolation since during acquisition,
	   no automatic update of the statistic estimates is done. We need that
	   because we set the correct parameters once in the init routine */
	ChannelEstimation.GetTimeWiener()->StopTracking();

	/* Disable FAC evaluation to make sure that no mistakenly correct CRC
	   sets false parameters which can cause run-time errors */
	UtilizeFACData.SetSyncInput(true);

	/* We have to first start aquisition and then stop it right after it to set
	   internal parameters */
	SyncUsingPil.StartAcquisition();
	SyncUsingPil.StopAcquisition();

	SyncUsingPil.StartTrackPil();
}


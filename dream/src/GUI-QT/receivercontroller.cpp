#include "ReceiverController.h"

ReceiverController::ReceiverController(CDRMReceiver* p, CSettings& s, QObject *parent) :
    QObject(parent),
    timer(),
    receiver(p),
    iCurrentFrequency(-1),
    currentMode(RM_NONE),
    settings(s)
{
    setObjectName("controller");
    connect(&timer, SIGNAL(timeout()), SLOT(on_timer()));
}

void ReceiverController::setControls()
{
    // initialise from settings here so multiple panels are synced
    emit numMSCMLCIterationsChanged(settings.Get("Receiver", "mlciter", 1));
    emit timeIntChanged(settings.Get("Receiver", "timeint", 1));
    emit freqIntChanged(settings.Get("Receiver", "freqint", 1));
    emit tiSyncTracTypeChanged(settings.Get("Receiver", "timesync", 1));

    emit recFilterChanged(settings.Get("Receiver", "filter", FALSE));
    emit intConsChanged(settings.Get("Receiver", "modemetric", FALSE));
    emit flippedSpectrumChanged(settings.Get("Receiver", "flipspectrum", FALSE));
}

void ReceiverController::on_timer()
{
    CParameter& Parameters = *receiver->GetParameters();
    Parameters.Lock();

    /* Input level meter */
    double sigstr = Parameters.GetIFSignalLevel();

    /* LEDs */
    ETypeRxStatus msc,sdc,fac,fsync,tsync,inp,outp;
    fac = Parameters.ReceiveStatus.FAC.GetStatus();
    sdc = Parameters.ReceiveStatus.SDC.GetStatus();
    int iShortID = Parameters.GetCurSelAudioService();
    if(Parameters.Service[iShortID].eAudDataFlag == CService::SF_AUDIO)
        msc = Parameters.AudioComponentStatus[iShortID].GetStatus();
    else
        msc = Parameters.DataComponentStatus[iShortID].GetStatus();
    fsync = Parameters.ReceiveStatus.FSync.GetStatus();
    tsync = Parameters.ReceiveStatus.TSync.GetStatus();
    inp = Parameters.ReceiveStatus.InterfaceI.GetStatus();
    outp = Parameters.ReceiveStatus.InterfaceO.GetStatus();

    /* detect if AFS mux information is available */
    bool bAFS = (Parameters.AltFreqSign.vecMultiplexes.size() > 0)
             || (Parameters.AltFreqSign.vecOtherServices.size() > 0);
    gps_data_t gps_data = Parameters.gps_data;

    map<uint32_t,CServiceInformation> si = Parameters.ServiceInformation;

    Reception reception;

    reception.snr = Parameters.GetSNR();
    reception.mer = Parameters.rMER;
    reception.wmer = Parameters.rWMERMSC;
    reception.sigmaEstimate = Parameters.rSigmaEstimate;
    reception.minDelay = Parameters.rMinDelay;
    reception.sampleOffset = Parameters.rResampleOffset;
    reception.sampleRate = Parameters.GetSigSampleRate();
    reception.dcOffset = receiver->GetReceiveData()->ConvertFrequency(Parameters.GetDCFrequency());
    reception.rdop = Parameters.rRdop;

    ChannelConfiguration channel;

    channel.robm = Parameters.GetWaveMode();
    channel.mode = Parameters.GetSpectrumOccup();
    channel.interl = Parameters.eSymbolInterlMode;
    channel.sdcConst = Parameters.eSDCCodingScheme;
    channel.mscConst = Parameters.eMSCCodingScheme;
    channel.protLev = Parameters.MSCPrLe;

    Parameters.Unlock();

    emit InputSignalLevelChanged(sigstr);
    emit FACChanged(fac);
    emit SDCChanged(sdc);
    emit MSCChanged(msc);
    emit FSyncChanged(fsync);
    emit TSyncChanged(tsync);
    emit InputStatusChanged(inp);
    emit OutputStatusChanged(outp);

    emit setAFS(bAFS);
    // NB following is not efficient - TODO - have a better idea
    if(bAFS)
        emit AFS(Parameters.AltFreqSign);

    if(gps_data.status&LATLON_SET)
        emit position(gps_data.fix.latitude, gps_data.fix.longitude);

    emit serviceInformation(si);

    ERecMode newMode = receiver->GetReceiverMode();
    if(newMode!=currentMode)
    {
        currentMode = newMode;
        emit mode(currentMode);
    }

    /* Check if receiver does receive a signal */
    if(receiver->GetAcquiState() == AS_WITH_SIGNAL)
    {
        switch(Parameters.GetReceiverMode())
        {
        case RM_DRM:
            updateDRM(Parameters);
        default:
            ;
        }

        emit channelReceptionChanged(reception);
        emit channelConfigurationChanged(channel);

        int iFreq = receiver->GetFrequency();
        if(iFreq != iCurrentFrequency)
        {
            emit frequencyChanged(iFreq);
            iCurrentFrequency = iFreq;
        }
    }
    else
    {
        emit signalLost();
    }

    /* Time, date ####################
    if ((Parameters.iUTCHour == 0) &&
            (Parameters.iUTCMin == 0) &&
            (Parameters.iDay == 0) &&
            (Parameters.iMonth == 0) &&
            (Parameters.iYear == 0))
            */
}



void ReceiverController::updateDRM(CParameter& Parameters)
{
    for(int i=0; i < MAX_NUM_SERVICES; i++)
    {
        Parameters.Lock();
        CService& service = Parameters.Service[i];
        service.AudioParam.rBitRate = Parameters.GetBitRateKbps(i, false);
        service.AudioParam.bCanDecode = receiver->GetAudSorceDec()->CanDecode(service.AudioParam.eAudioCoding);
        service.DataParam.rBitRate = Parameters.GetBitRateKbps(i, true);
        service.DataParam.pDecoder = receiver->GetDataDecoder();
        ETypeRxStatus e = Parameters.DataComponentStatus[i].GetStatus();
        Parameters.Unlock();

        emit serviceChanged(i, service); // TODO emit only when changed

        /* If we have text messages */
        QString textMessage = "";
        if (service.AudioParam.bTextflag)
        {
            // Text message of current selected audio service (UTF-8 decoding)
            textMessage = QString::fromUtf8(service.AudioParam.strTextMessage.c_str());
        }
        emit textMessageChanged(i, textMessage);

        if(service.DataParam.iStreamID != STREAM_ID_NOT_USED)
            emit dataStatusChanged(i, e);
    }
}

void ReceiverController::setFrequency(int f)
{
    receiver->SetFrequency(f);
}

void ReceiverController::triggerNewAcquisition()
{
    receiver->RequestNewAcquisition();
}

void ReceiverController::setMode(int newMode)
{
    receiver->SetReceiverMode(ERecMode(newMode));
}

void ReceiverController::selectAudioService(int shortId)
{
    CParameter& Parameters = *receiver->GetParameters();
    Parameters.Lock();
    Parameters.SetCurSelAudioService(shortId);
    Parameters.Unlock();
}

void ReceiverController::selectDataService(int shortId)
{
    CParameter& Parameters = *receiver->GetParameters();
    Parameters.Lock();
    Parameters.SetCurSelDataService(shortId);
    Parameters.Unlock();
}
void ReceiverController::setSaveAudio(const string& s)
{
    if(s!="")
        receiver->GetWriteData()->StartWriteWaveFile(s);
    else
        receiver->GetWriteData()->StopWriteWaveFile();
}

void ReceiverController::muteAudio(bool b)
{
    receiver->GetWriteData()->MuteAudio(b);
}

void ReceiverController::setTimeInt(int v)
{
    CChannelEstimation::ETypeIntTime e = CChannelEstimation::ETypeIntTime(v);
    if (receiver->GetTimeInt() != e)
        receiver->SetTimeInt(e);
}

void ReceiverController::setFreqInt(int v)
{
    CChannelEstimation::ETypeIntFreq e = CChannelEstimation::ETypeIntFreq(v);
    if (receiver->GetFreqInt() != e)
        receiver->SetFreqInt(e);
}

void ReceiverController::setTiSyncTracType(int v)
{
    CTimeSyncTrack::ETypeTiSyncTrac e = CTimeSyncTrack::ETypeTiSyncTrac(v);
    if (receiver->GetTiSyncTracType() != e)
        receiver->SetTiSyncTracType(e);
}

void ReceiverController::setNumMSCMLCIterations(int value)
{
    receiver->GetMSCMLC()->SetNumIterations(value);
}

void ReceiverController::setFlippedSpectrum(bool b)
{
    receiver->GetReceiveData()->SetFlippedSpectrum(b);
}

void ReceiverController::setReverbEffect(bool b)
{
    receiver->GetAudSorceDec()->SetReverbEffect(b);
}

void ReceiverController::setRecFilter(bool b)
{
    /* Set parameter in working thread module */
    receiver->GetFreqSyncAcq()->SetRecFilter(b);

    /* If filter status is changed, a new aquisition is necessary */
    receiver->RequestNewAcquisition();
}

void ReceiverController::setIntCons(bool b)
{
    /* Set parameter in working thread module */
    receiver->SetIntCons(b);
}

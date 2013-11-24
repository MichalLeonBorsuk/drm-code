#include "ReceiverController.h"

ReceiverController::ReceiverController(CDRMReceiver* p, QObject *parent) :
    QObject(parent),
    timer(),
    receiver(p),
    iCurrentFrequency(-1),
    currentMode(RM_NONE)
{
    setObjectName("controller");
    connect(&timer, SIGNAL(timeout()), SLOT(on_timer()));
}

void ReceiverController::on_timer()
{
    CParameter& Parameters = *receiver->GetParameters();
    Parameters.Lock();

    /* Input level meter */
    double sigstr = Parameters.GetIFSignalLevel();
    double rWMERMSC = Parameters.rWMERMSC;

    /* LEDs */
    ETypeRxStatus msc,sdc,fac;
    fac = Parameters.ReceiveStatus.FAC.GetStatus();
    sdc = Parameters.ReceiveStatus.SDC.GetStatus();
    int iShortID = Parameters.GetCurSelAudioService();
    if(Parameters.Service[iShortID].eAudDataFlag == CService::SF_AUDIO)
        msc = Parameters.AudioComponentStatus[iShortID].GetStatus();
    else
        msc = Parameters.DataComponentStatus[iShortID].GetStatus();
    /* detect if AFS mux information is available */
    bool bAFS = (Parameters.AltFreqSign.vecMultiplexes.size() > 0)
             || (Parameters.AltFreqSign.vecOtherServices.size() > 0);
    gps_data_t gps_data = Parameters.gps_data;

    map<uint32_t,CServiceInformation> si = Parameters.ServiceInformation;

    Parameters.Unlock();

    emit WMERChanged(rWMERMSC);
    emit InputSignalLevelChanged(sigstr);
    emit FACChanged(fac);
    emit SDCChanged(sdc);
    emit MSCChanged(msc);
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

void ReceiverController::setTimeInt(CChannelEstimation::ETypeIntTime e)
{
    if (receiver->GetTimeInt() != e)
        receiver->SetTimeInt(e);
}

void ReceiverController::setFreqInt(CChannelEstimation::ETypeIntFreq e)
{
    if (receiver->GetFreqInt() != e)
        receiver->SetFreqInt(e);
}

void ReceiverController::setTiSyncTracType(CTimeSyncTrack::ETypeTiSyncTrac e)
{
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

#ifndef RECEIVERCONTROLLER_H
#define RECEIVERCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <../DrmReceiver.h>

class ReceiverController : public QObject
{
    Q_OBJECT
public:
    explicit ReceiverController(CDRMReceiver*, QObject *parent = 0);

signals:
    void serviceChanged(int, const CService&);
    void dataStatusChanged(int, ETypeRxStatus);
    void frequencyChanged(int);
    void mode(int);
    void position(double,double);
    void AFS(const CAltFreqSign&);
    void setAFS(bool);
    void serviceInformation(const map <uint32_t,CServiceInformation>);
    void textMessageChanged(int, const QString&);
    void MSCChanged(ETypeRxStatus);
    void SDCChanged(ETypeRxStatus);
    void FACChanged(ETypeRxStatus);
    void WMERChanged(double);
    void InputSignalLevelChanged(double);
    void signalLost();

public slots:
    void start(int ms) { timer.start(ms); }
    void stop() { timer.stop(); }
    void selectDataService(int);
    void selectAudioService(int);
    void triggerNewAcquisition();
    void setMode(int);
    void setFrequency(int);
    void muteAudio(bool);
    void setSaveAudio(const string&);
    void setTimeInt(CChannelEstimation::ETypeIntTime);
    void setFreqInt(CChannelEstimation::ETypeIntFreq);
    void setTiSyncTracType(CTimeSyncTrack::ETypeTiSyncTrac);
    void setNumMSCMLCIterations(int);
    void setFlippedSpectrum(bool);
    void setReverbEffect(bool);
    void setRecFilter(bool);
    void setIntCons(bool);

private:
    QTimer        timer;
    CDRMReceiver* receiver;
    int           iCurrentFrequency;
    ERecMode      currentMode;

private slots:
    void on_timer();
    void updateDRM(CParameter&);
};

#endif // RECEIVERCONTROLLER_H

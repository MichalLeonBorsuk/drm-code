#ifndef RECEIVERCONTROLLER_H
#define RECEIVERCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <../DrmReceiver.h>
#include <../util/Settings.h>

struct Reception {
    double snr, mer, wmer;
    double sigmaEstimate;
    double minDelay;
    double sampleOffset;
    int sampleRate;
    double dcOffset;
    double rdop;
};

struct ChannelConfiguration {
    int robm;
    int mode;
    int interl;
    int sdcConst, mscConst;
    CMSCProtLev protLev;
};

class ReceiverController : public QObject
{
    Q_OBJECT
public:
    explicit ReceiverController(CDRMReceiver*, CSettings&, QObject *parent = 0);
    // TODO - remove this:
    CDRMReceiver* getReceiver() { return receiver; }

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
    void FSyncChanged(ETypeRxStatus);
    void TSyncChanged(ETypeRxStatus);
    void InputStatusChanged(ETypeRxStatus);
    void OutputStatusChanged(ETypeRxStatus);
    void InputSignalLevelChanged(double);
    void signalLost();
    void channelReceptionChanged(Reception);
    void channelConfigurationChanged(ChannelConfiguration);
    void timeIntChanged(int);
    void freqIntChanged(int);
    void tiSyncTracTypeChanged(int);
    void numMSCMLCIterationsChanged(int);
    void flippedSpectrumChanged(bool);
    void recFilterChanged(bool);
    void intConsChanged(bool);

public slots:
    void start(int ms) { timer.start(ms); }
    void stop() { timer.stop(); }
    void setControls();
    void selectDataService(int);
    void selectAudioService(int);
    void triggerNewAcquisition();
    void setMode(int);
    void setFrequency(int);
    void muteAudio(bool);
    void setSaveAudio(const string&);
    void setTimeInt(int);
    void setFreqInt(int);
    void setTiSyncTracType(int);
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
    CSettings&    settings;

private slots:
    void on_timer();
    void updateDRM(CParameter&);
};

#endif // RECEIVERCONTROLLER_H

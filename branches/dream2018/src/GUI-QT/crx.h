#ifndef CRX_H
#define CRX_H

#include "ctrx.h"
#include <QObject>
#include "../Parameter.h"

class CDRMReceiver;
class CPlotManager;
class CDataDecoder;

class CRx : public CTRx
{
    Q_OBJECT
public:
    explicit CRx(CDRMReceiver& nRx, CTRx *parent = nullptr);
    virtual ~CRx() override;

    virtual void    GetInputPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    virtual void    GetPowDenSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    virtual void    GetInputSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    virtual void    GetAudioSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    virtual void    GetAMBWParameters(double& rCenterFreq, double& rBW);
    virtual int     GetAMNoiseReductionLevel();
    virtual ENoiRedType GetAMNoiseReductionType();
    virtual EDemodType  GetAMDemodulationType();
    virtual EAmAgcType GetAMAGCType();
    virtual int GetAMFilterBW();
    virtual double GetAMMixerFrequencyOffset() const;
    virtual bool GetAMPLLPhase(_REAL&);

    virtual ERecMode GetReceiverMode();
    virtual EInChanSel GetInChanSel();
    virtual int GetMSCMLInitNumIterations();
    virtual CPlotManager* GetPlotManager();
    _REAL ConvertFrequency(_REAL rFrequency, bool bInvert=false) const;
    virtual void GetMSCMLCVectorSpace(CVector<_COMPLEX>&);
    virtual void GetSDCMLCVectorSpace(CVector<_COMPLEX>&);
    virtual void GetFACMLCVectorSpace(CVector<_COMPLEX>&);

    virtual bool inputIsRSCI();
    virtual bool isWriteWaveFile();
    virtual bool isAudioMuted();
    virtual bool isIntefererConsiderationEnabled();
    virtual bool isFrequencySyncAcquisitionFilterEnabled() const;
    virtual bool isSpectrumFlipped();
    virtual bool isAMAutoFrequencyAcquisitionEnabled();
    virtual bool isAMPLLEnabled();
    virtual bool GetAMSSPLLPhase(_REAL&);
    virtual int GetAMSSPercentageDataEntityGroupComplete();
    virtual char* GetAMSSDataEntityGroupStatus();
    virtual int GetAMSSCurrentBlock();
    virtual char* GetAMSSCurrentBlockBits();
    virtual bool GetAMSSBlock1Status();
    virtual EAMSSBlockLockStat GetAMSSLockStatus();

    virtual bool CanDecode(int);

    virtual ETypeIntTime GetTimeInterpolationAlgorithm() const;
    virtual ETypeIntFreq GetFrequencyInterpolationAlgorithm() const;
    virtual ETypeTiSyncTrac GetTimeSyncTrackingType();

    virtual CDataDecoder* GetDataDecoder();
    virtual bool GetReverbEffect();
    virtual EAcqStat GetAcquisitionState();
    virtual int GetFrequency() override;

public slots:
    virtual void LoadSettings() override;
    virtual void SaveSettings() override;
    virtual void SetInputDevice(QString) override;
    virtual void SetOutputDevice(QString) override;
    virtual void GetInputDevice(string&) override;
    virtual void GetOutputDevice(string&) override;
    virtual void EnumerateInputs(std::vector<std::string>& names, std::vector<std::string>& descriptions) override;
    virtual void EnumerateOutputs(std::vector<std::string>& names, std::vector<std::string>& descriptions) override;
    virtual void Start() override;
    virtual void Restart() override;
    virtual void Stop() override;
    virtual CSettings*				GetSettings() override;
    virtual void					SetSettings(CSettings* pNewSettings) override;
    virtual CParameter*				GetParameters() override;
    virtual _BOOLEAN				IsReceiver() const override { return true;}
    virtual _BOOLEAN				IsTransmitter() const override { return false;}
    virtual void StartWriteWaveFile(string);
    virtual void StopWriteWaveFile();
    virtual void SetTimeInterpolationAlgorithm(ETypeIntTime);
    virtual void SetFrequencyInterpolationAlgorithm(ETypeIntFreq);
    virtual void SetTimeSyncTrackingType(ETypeTiSyncTrac);
    virtual void SetNumMSCMLCIterations(int);
    virtual void SetFlipSpectrum(bool);
    virtual void SetFrequencySyncAcquisitionFilter(bool);
    virtual void SetConsiderInterferer(bool);
    virtual void MuteAudio(bool);
    virtual void SetReverberationEffect(bool);
    virtual void SetReceiverMode(ERecMode);
    virtual void SetAMDemodulationType(EDemodType);
    virtual void SetAMFilterBW(int);
    virtual void SetAMAGCType(EAmAgcType);
    virtual void SetAMNoiseReductionType(ENoiRedType);
    virtual void SetAMNoiseReductionLevel(int);
    virtual void SetFrequency(int) override;
    virtual void SetAMDemodAcq(_REAL);
    virtual void EnableAMPLL(bool);
    virtual void EnableAutoFrequenctAcquisition(bool);

private:
    CDRMReceiver& rx;
};

#endif // CRX_H

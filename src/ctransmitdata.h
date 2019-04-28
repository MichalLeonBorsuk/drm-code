#ifndef CTRANSMITDATA_H
#define CTRANSMITDATA_H

#include "util/Modul.h"
#include "sound/soundinterface.h"
#include "util/Utilities.h"
#ifdef QT_MULTIMEDIA_LIB
# include <QIODevice>
#endif

class CTransmitData : public CTransmitterModul<_COMPLEX, _COMPLEX>
{
public:
    enum EOutFormat {OF_REAL_VAL /* real valued */, OF_IQ_POS,
                     OF_IQ_NEG /* I / Q */, OF_EP /* envelope / phase */
                    };

    CTransmitData();

    virtual ~CTransmitData();

    void SetIQOutput(const EOutFormat eFormat) {
        eOutputFormat = eFormat;
    }
    EOutFormat GetIQOutput() {
        return eOutputFormat;
    }

    void SetAmplifiedOutput(bool bEnable) {
        bAmplified = bEnable;
    }
    bool GetAmplifiedOutput() {
        return bAmplified;
    }

    void SetHighQualityIQ(bool bEnable) {
        bHighQualityIQ = bEnable;
    }
    bool GetHighQualityIQ() {
        return bHighQualityIQ;
    }

    void SetCarOffset(const CReal rNewCarOffset)
    {
        rDefCarOffset = rNewCarOffset;
    }

    void SetSoundInterface(std::string);
    std::string GetSoundInterface() { return soundDevice; }
    void Enumerate(std::vector<string>& names, std::vector<string>& descriptions);
    void Stop();
#ifdef QT_MULTIMEDIA_LIB
    std::string GetSoundInterfaceVersion() { return "QtMultimedia"; }
#else
    std::string GetSoundInterfaceVersion() { return pSound->GetVersion(); }
#endif

    void SetWriteToFile(const std::string strNFN)
    {
        strOutFileName = strNFN;
        bUseSoundcard = false;
    }

    void FlushData();

protected:
    FILE*			pFileTransmitter;
#ifdef QT_MULTIMEDIA_LIB
    QIODevice*      pIODevice;
#endif
    CSoundOutInterface*	pSound;
    std::string     soundDevice;
    CVector<short>	vecsDataOut;
    int				iBlockCnt;
    int				iNumBlocks;
    EOutFormat		eOutputFormat;

    CDRMBandpassFilt	BPFilter;
    CReal			rDefCarOffset;

    CReal			rNormFactor;

    int				iBigBlockSize;

    std::string		strOutFileName;
    bool			bUseSoundcard;
    int				iSampleRate;

    bool			bAmplified;
    bool			bHighQualityIQ;
    CVector<_REAL>	vecrReHist;

    void HilbertFilt(_COMPLEX& vecData);

    virtual void InitInternal(CParameter& TransmParam);
    virtual void ProcessDataInternal(CParameter& Parameter);
};


#endif // CTRANSMITDATA_H

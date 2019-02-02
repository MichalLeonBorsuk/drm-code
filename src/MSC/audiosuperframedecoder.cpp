#include "audiosuperframedecoder.h"
#include <iostream>
#include "xheaacsuperframe.h"
#include "aacsuperframe.h"

AudioSuperframeDecoder::AudioSuperframeDecoder():pAudioSuperFrame(nullptr)
{

}

AudioSuperframeDecoder::~AudioSuperframeDecoder()
{
    if(pAudioSuperFrame != nullptr) delete pAudioSuperFrame;
}

AudioFrame& AudioSuperframeDecoder::getFrame(unsigned i)
{
    AudioFrame* af = new AudioFrame();
    if(pAudioSuperFrame != nullptr)
        pAudioSuperFrame->getFrame(af->samples, af->crc, i);
    return *af;
}


void AudioSuperframeDecoder::InitInternal(CParameter& Parameters)
{
    cerr << "AudioSuperframeDecoder::InitInternal" << endl;

    /* Get current selected audio service */
    int iCurSelServ = Parameters.GetCurSelAudioService();

    if ((Parameters.Service[iCurSelServ].eAudDataFlag != CService::SF_AUDIO))
    {
        cerr << "AudioSuperframeDecoder called with data service" << endl;
        return; // TODO throw CInitErr(ET_ALL);
    }

    /* Get current selected audio param */
    CAudioParam& AudioParam(Parameters.Service[iCurSelServ].AudioParam);

    /* Current audio stream ID */
    int iCurAudioStreamID = AudioParam.iStreamID;

    if(iCurAudioStreamID == STREAM_ID_NOT_USED)
    {
        cerr << "AudioSuperframeDecoder called with unused stream" << endl;
        return;
    }

    /* Get number of total input bits for this module */
    iInputBlockSize = Parameters.iNumAudioDecoderBits;

    int iTotalFrameSize = Parameters.Stream[iCurAudioStreamID].iLenPartA+Parameters.Stream[iCurAudioStreamID].iLenPartB;

    /* Init text message application ------------------------------------ */
    if (AudioParam.bTextflag)
    {
        iTotalFrameSize -= NUM_BYTES_TEXT_MESS_IN_AUD_STR;
    }

    if(pAudioSuperFrame != nullptr) delete pAudioSuperFrame;
    if(AudioParam.eAudioCoding==CAudioParam::AC_xHE_AAC) {
        XHEAACSuperFrame* p = new XHEAACSuperFrame();
        // part B should be enough as xHE-AAC MUST be EEP but its easier to just add them
        p->init(AudioParam, iTotalFrameSize);
        pAudioSuperFrame = p;
    }
    else {
        AACSuperFrame *p = new AACSuperFrame();
        p->init(AudioParam,  Parameters.GetWaveMode(), Parameters.Stream[iCurAudioStreamID].iLenPartA, Parameters.Stream[iCurAudioStreamID].iLenPartB);
        //delete p;
        pAudioSuperFrame = p;
    }
}

void AudioSuperframeDecoder::ProcessDataInternal(CParameter& Parameters)
{
    if(pAudioSuperFrame == nullptr) {
        cerr << "AudioSuperframeDecoder called when not initialised" << endl;
        return;
    }
    /* Reset bit extraction access */
    (*pvecInputData).ResetBitAccess();

    bool bGoodValues = pAudioSuperFrame->parse(*pvecInputData);
    if(bGoodValues) {
        cerr << "got audio superframe with " << pAudioSuperFrame->getNumFrames() << " audio frames" << endl;
    }
    else {
        cerr << "error parsing audio superframe" << endl;
    }
}

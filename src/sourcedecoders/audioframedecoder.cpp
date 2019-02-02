#include "audioframedecoder.h"
#include "../util/Vector.h"
#include <QDebug>

AudioFrameDecoder::AudioFrameDecoder(QObject *parent) : QObject(parent),pCodec(nullptr)
{

}

void AudioFrameDecoder::on_rxSignaller_audioConfigSignalled(const CAudioParam& audioParam)
{
    //qDebug() << "AudioFrameDecoder::on_rxSignaller_audioConfigSignalledg";
    /* Get decoder instance */
    pCodec = CAudioCodec::GetDecoder(audioParam.eAudioCoding);
    pCodec->DecOpen(audioParam, sampleRate);
    if(sampleRate==0) {
        qDebug() << "no sample rate from DecOpen";
    }
    else {
        qDebug() << "sample rate" << sampleRate;
    }
}

void AudioFrameDecoder::decodeFrame(AudioFrame audioFrame)
{
    if(sampleRate==0)
        return;
    vector<short> samples;
    int channels;
    CAudioCodec::EDecError eDecError = pCodec->Decode(audioFrame.samples, audioFrame.crc, samples, channels);
    if(eDecError==CAudioCodec::DECODER_ERROR_OK) {
        QByteArray ba(reinterpret_cast<char*>(&samples[0]), int(samples.size()*sizeof (samples[0])));
        QAudioFormat format;
        format.setSampleRate(sampleRate);
        format.setChannelCount(channels);
        format.setSampleType(QAudioFormat::SignedInt);
        format.setSampleSize(16);
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setCodec("audio/pcm");
        QAudioBuffer buf(ba, format);
        emit audioDecoded(buf);
    }
    else {
        qDebug() << "AudioFrameDecoder::decodeFrame bad";
    }

}

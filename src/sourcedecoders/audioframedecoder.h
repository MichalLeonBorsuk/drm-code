#ifndef AUDIOFRAMEDECODER_H
#define AUDIOFRAMEDECODER_H

#include <QObject>
#include <QAudioBuffer>
#include "audioframe.h"
#include "../SDC/audioparam.h"
#include "AudioCodec.h"

class AudioFrameDecoder : public QObject
{
    Q_OBJECT
public:
    explicit AudioFrameDecoder(QObject *parent = nullptr);

signals:
    void audioDecoded(const QAudioBuffer&);

public slots:
    void on_rxSignaller_audioConfigSignalled(const CAudioParam&);
    void decodeFrame(AudioFrame);
private:
    CAudioCodec* pCodec;
    int sampleRate;
};

#endif // AUDIOFRAMEDECODER_H

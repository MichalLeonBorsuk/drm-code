#include "audioframereceiver.h"

AudioFrameReceiver::AudioFrameReceiver(QObject *parent) : QObject(parent)
{

}

void AudioFrameReceiver::decodeFrame(AudioFrame audioFrame)
{
    emit frameReceived(audioFrame);
}

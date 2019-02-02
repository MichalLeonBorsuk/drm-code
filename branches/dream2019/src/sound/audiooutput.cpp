#include "audiooutput.h"
#include <QAudioDeviceInfo>
#include <QDebug>

AudioOutput::AudioOutput(QObject *parent) : QObject(parent),pOutput(nullptr),pDevice(nullptr)
{
}

void AudioOutput::audioDecoded(const QAudioBuffer& buf)
{
    //qDebug() << "output";
    if(pDevice==nullptr) {
        initialise(buf.format());
    }
    if(pDevice!=nullptr && pDevice->isWritable()) {
        pDevice->write(buf.constData<char>(), buf.byteCount());
    }
}

void AudioOutput::initialise(const QAudioFormat& format, QString deviceName)
{
    if(pOutput==nullptr) {
        foreach(const QAudioDeviceInfo& di, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
            if(deviceName == "" || deviceName == di.deviceName()) {
                pOutput = new QAudioOutput(di, format);
                break;
            }
        }
    }
    if(pOutput!=nullptr) {
        pOutput->setBufferSize(1000000);
        pDevice = pOutput->start();
        if(pDevice==nullptr) {
            qDebug() << "can't start audio output" << pOutput->error();
        }
    }
    else {
        qDebug("can't make audio output");
    }
}

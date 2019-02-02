#include "rxsignaller.h"

RxSignaller::RxSignaller(QObject *parent) : QObject(parent)
{

}

void RxSignaller::signalAudioConfig(const CAudioParam& audioParam)
{
    emit audioConfigSignalled(audioParam);
}

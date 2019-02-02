#ifndef AUDIOFRAMERECEIVER_H
#define AUDIOFRAMERECEIVER_H

#include <QObject>
#include "audioframe.h"

class AudioFrameReceiver : public QObject
{
    Q_OBJECT
public:
    explicit AudioFrameReceiver(QObject *parent = nullptr);
    void decodeFrame(AudioFrame);

signals:
    void frameReceived(AudioFrame);

public slots:
};

#endif // AUDIOFRAMERECEIVER_H

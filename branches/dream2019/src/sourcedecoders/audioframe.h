#ifndef AUDIOFRAME_H
#define AUDIOFRAME_H

#include <vector>
#include <QMetaType>

class AudioFrame
{
public:
    AudioFrame();
    std::vector<uint8_t> samples;
    uint8_t crc;
};

Q_DECLARE_METATYPE(AudioFrame)

#endif // AUDIOFRAME_H

#ifndef XHEAACSUPERFRAME_H
#define XHEAACSUPERFRAME_H

#include "audiosuperframe.h"
#include "frameborderdescription.h"

class XHEAACSuperFrame: public AudioSuperFrame
{
public:
    XHEAACSuperFrame();
    void init(const CAudioParam& audioParam, unsigned frameSize);
    virtual bool parse(CVectorEx<_BINARY>& asf);
    virtual unsigned getNumFrames() { return audioFrame.size(); }
    virtual void getFrame(std::vector<uint8_t>& frame, uint8_t& crc, unsigned i) { }
private:
    unsigned numChannels;
    unsigned frameSize;
    uint8_t previous[2];
    std::vector<uint8_t> partialFrame;
};

#endif // XHEAACSUPERFRAME_H

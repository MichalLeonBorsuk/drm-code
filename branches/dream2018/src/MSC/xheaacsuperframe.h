#ifndef XHEAACSUPERFRAME_H
#define XHEAACSUPERFRAME_H

#include "audiosuperframe.h"
#include "frameborderdescription.h"

class XHEAACSuperFrame: public AudioSuperFrame
{
public:
    XHEAACSuperFrame();
    virtual bool parse(CVectorEx<_BINARY>& asf);
    virtual unsigned getNumFrames() { return frameBorderCount+1; }
    virtual void getFrame(std::vector<uint8_t>& frame, uint8_t& crc, unsigned i) { }
private:
    // Header section
    unsigned frameBorderCount;
    unsigned bitReservoirLevel;
    unsigned fixedHeaderCRC;
    // Payload section
    uint8_t previous[2];
    // Directory section
    std::vector<FrameBorderDescription> frameBorderDescription;
};

#endif // XHEAACSUPERFRAME_H

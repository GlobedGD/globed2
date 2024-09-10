#pragma once

#include <defs/minimal_geode.hpp>

class ByteBuffer;

class UdpFrameBuffer {
public:
    // push a new frame, if a packet is completed, return the full data
    Result<std::vector<uint8_t>> pushFrameFromBuffer(ByteBuffer& buf);

    void clear();
private:
    struct Frame {
        uint8_t idx;
        uint8_t max;
        std::vector<uint8_t> data;
    };

    std::map<int, std::vector<Frame>> packets;
};

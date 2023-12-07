#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include "opus_codec.hpp"

class AudioSampleQueue {
public:
    void writeData(DecodedOpusData data);
    void writeData(float* pcm, size_t length);
    // contrary to the name, this will erase the samples from this queue after copying them to `dest`
    size_t copyTo(float* dest, size_t samples);
    size_t size() const;
    void clear();

private:
    std::vector<float> buf;
};

#endif // GLOBED_VOICE_SUPPORT

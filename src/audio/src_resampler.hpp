#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include <util/data.hpp>
#include <samplerate.h>

class SRCResampler {
public:
    double samplerRatio = 0.0;
    
    SRCResampler(int sourceRate = 0, int outRate = 0);
    ~SRCResampler();

    // length is in samples. the returned pointer is guaranteed to be valid
    // until the next call to `resample` or `setSampleRate`.
    // you must not manually free the returned pointer.
    [[nodiscard]] float* resample(const float* source, size_t length);

    // sets the sample rate and recreates the resampler state
    void setSampleRate(int sampleRate, int outRate);

protected:
    SRC_STATE* srcState = nullptr;
    int sourceRate, outRate;
    int _err = 0;

    float* buffer = nullptr;

    size_t bufferLength = 0; // in samples

    void verifyBufferSize(size_t required);
    void errcheck();
    void cleanup();
};

#endif // GLOBED_VOICE_SUPPORT
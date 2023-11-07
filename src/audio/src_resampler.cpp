#include "src_resampler.hpp"

#if GLOBED_VOICE_SUPPORT

const int RESAMPLER_QUALITY = SRC_LINEAR;
// 10x slower on average, barely any difference in quality
// const int RESAMPLER_QUALITY = SRC_SINC_FASTEST;
const size_t OUT_EXTRA_FRAMES = 16;

SRCResampler::SRCResampler(int sourceRate, int outRate) {
    setSampleRate(sourceRate, outRate);
}

SRCResampler::~SRCResampler() {
    cleanup();
}

float* SRCResampler::resample(const float* source, size_t length) {
    verifyBufferSize(length);

    _err = src_reset(srcState);
    errcheck();

    _err = src_set_ratio(srcState, this->samplerRatio);
    errcheck();

    // resample
    SRC_DATA data;
    data.data_in = source;
    data.data_out = buffer;
    data.input_frames = static_cast<long>(length);
    data.output_frames = static_cast<long>(length * samplerRatio + OUT_EXTRA_FRAMES);
    data.src_ratio = this->samplerRatio;

    _err = src_process(srcState, &data);
    errcheck();

    return buffer;
}

void SRCResampler::setSampleRate(int sampleRate, int outRate) {
    this->sourceRate = sampleRate;
    this->outRate = outRate;

    cleanup();

    if (sampleRate == 0) {
        this->samplerRatio = 0.0;
        return;
    }

    this->samplerRatio = static_cast<double>(outRate) / static_cast<double>(sampleRate);

    srcState = src_new(RESAMPLER_QUALITY, 1, &_err);
    errcheck();

    _err = src_set_ratio(srcState, this->samplerRatio);
    errcheck();
}

void SRCResampler::verifyBufferSize(size_t required) {
    if (bufferLength == 0 || buffer == nullptr || required < bufferLength) {
        if (buffer)
            delete[] buffer;
        
        buffer = new float[required * samplerRatio + OUT_EXTRA_FRAMES]; // 16 for safety
        bufferLength = required;
    }
}

void SRCResampler::errcheck() {
    if (_err != 0) {
        const char* msg = src_strerror(_err);
        GLOBED_ASSERT(false, std::string("src error: ") + msg);
    }
}

void SRCResampler::cleanup() {
    if (srcState) {
        srcState = src_delete(srcState);
    }

    bufferLength = 0;

    if (buffer) {
        delete[] buffer;
        buffer = nullptr;
    }
}

#endif // GLOBED_VOICE_SUPPORT
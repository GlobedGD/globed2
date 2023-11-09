#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include <util/data.hpp>
#include <opus.h>

struct EncodedOpusData {
    util::data::byte* ptr;
    int64_t length;
};

struct DecodedOpusData {
    float* ptr;
    size_t length;
};

class OpusCodec {
public:
    OpusCodec(int sampleRate = 0, int frameSize = 0);
    ~OpusCodec();

    // takes in an input sample rate and finds the nearest sample rate that opus accepts
    constexpr static int calcOutSampleRate(int inRate) {
        // opus accepts 8000, 12000, 16000, 24000, or 48000.

        // most common case optimization
        if (inRate >= 36000) {
            return 48000;
        }

        int rates[] = {8000, 12000, 16000, 24000, 48000};

        int closest = 8000;
        int diff = std::abs(inRate - closest);

        for (int rate : rates) {
            int curdiff = std::abs(inRate - rate);
            if (curdiff < diff) {
                diff = curdiff;
                closest = rate;
            }
        }

        return closest;
    }


    // Encodes the given PCM samples with opus. You are responsible for freeing the source,
    // the destination should be freed with `OpusCodec::freeData`.
    // the amount of samples must be the same as `frameSize` passed in the constructor, otherwise the behavior is undefined.
    [[nodiscard]] EncodedOpusData encode(const float* data);
    // Decodes the given opus data into PCM samples. You are responsible for freeing the source,
    // the destination should be freed with `OpusCodec::freeData`.
    // `length` argument indicates the size of the encoded data in bytes.
    [[nodiscard]] DecodedOpusData decode(const util::data::byte* data, size_t length);

    // Free the data held by this structure.
    static void freeData(EncodedOpusData data);
    // Free the data held by this structure.
    static void freeData(DecodedOpusData data);

    // sets the sample rate and recreates encoder and decoder
    void setSampleRate(int sampleRate);

    // sets the frame size
    void setFrameSize(int frameSize);

protected:
    OpusEncoder* encoder = nullptr;
    OpusDecoder* decoder = nullptr;
    int _res;
    int sampleRate, frameSize;
    const int channels = 1; // atm hardcoded, no reason to change

    void errcheck(const char* where);

    void cleanup();
};

#endif // GLOBED_VOICE_SUPPORT
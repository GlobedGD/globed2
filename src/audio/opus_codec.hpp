#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include <opus.h>

#include <util/data.hpp>
#include <data/bytebuffer.hpp>

const size_t VOICE_MAX_BYTES_IN_FRAME = 1000; // on avg 200 bytes but y'know, just in case

class EncodedOpusData {
public:
    util::data::byte* ptr;
    int64_t length;

    GLOBED_ENCODE {
        buf.writeByteArray(ptr, length);
    }

    GLOBED_DECODE {
        // when it comes to arbitrary allocation, DO NOT trust the data
        length = buf.readU32();
        GLOBED_REQUIRE(length <= VOICE_MAX_BYTES_IN_FRAME, fmt::format("Rejecting audio frame, size too large ({})", length))

        ptr = new util::data::byte[length];

        buf.readBytesInto(ptr, length);
    }
};

struct DecodedOpusData {
    float* ptr;
    size_t length;
};

class OpusCodec {
public:
    OpusCodec(int sampleRate = 0, int frameSize = 0);
    ~OpusCodec();

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
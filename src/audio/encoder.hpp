#pragma once
#include <defs/platform.hpp>

#if GLOBED_VOICE_SUPPORT

#include <defs/minimal_geode.hpp>
#include <data/bytebuffer.hpp>

constexpr size_t VOICE_MAX_BYTES_IN_FRAME = 1000;

struct OpusEncoder;

class EncodedOpusData {
public:
    util::data::byte* ptr;
    int64_t length;

    void freeData() {
        GLOBED_REQUIRE(ptr != nullptr, "attempting to double free an instance of EncodedOpusData")

        delete[] ptr;

#ifdef GLOBED_DEBUG
        // to try and prevent misuse
        ptr = nullptr;
        length = -1;
#endif // GLOBED_DEBUG
    }

    GLOBED_ENCODE {
        buf.writeByteArray(ptr, length);
    }

    GLOBED_DECODE {
        // when it comes to arbitrary allocation, DO NOT trust the sent data
        length = buf.readU32();
        GLOBED_REQUIRE(length <= VOICE_MAX_BYTES_IN_FRAME, fmt::format("Rejecting audio frame, size too large ({})", length))

        ptr = new util::data::byte[length];
        buf.readBytesInto(ptr, length);
    }
};

class AudioEncoder {
public:
    AudioEncoder(int sampleRate = 0, int frameSize = 0, int channels = 1);
    ~AudioEncoder();

    // disable copying and enable moving
    AudioEncoder(const AudioEncoder&) = delete;
    AudioEncoder& operator=(const AudioEncoder&) = delete;

    AudioEncoder(AudioEncoder&& other) noexcept;
    AudioEncoder& operator=(AudioEncoder&& other) noexcept;

    // Encode the given PCM samples with Opus. The amount of samples passed must be equal to `frameSize` passed in the constructor.
    // After you no longer need the encoded data, you must call `data.freeData()`, or (preferrably, for explicitness) `AudioEncoder::freeData(data)`
    [[nodiscard]] Result<EncodedOpusData> encode(const float* data);

    // Free the underlying buffer of the encoded frame
    static void freeData(EncodedOpusData& data) {
        data.freeData();
    }

    // sets the sample rate that will be used and recreates the encoder
    Result<> setSampleRate(int sampleRate);
    // sets the frame size of the data that will be used
    void setFrameSize(int frameSize);
    // sets the amount of channels that will be used and recreates the encoder
    Result<> setChannels(int channels);

private:
    // EXPERIMENTAL ZONE
    //
    // those functions are here for completeness sake, right now they are NOT used anywhere.
    // in future they may be configurable by the end user, in which case verify that the user cannot screw things up
    // and remove the `private:` specifier and this comment.

    // resets the internal state of the encoder
    Result<> resetState();

    // sets the bitrate for the encoder
    Result<> setBitrate(int bitrate);

    // sets the encoder complexity (1-10)
    Result<> setComplexity(int complexity);

    // sets whether to use VBR or CBR (if false)
    Result<> setVariableBitrate(bool variablebr = true);

protected:
    OpusEncoder* encoder = nullptr;

    int _res;
    int sampleRate, frameSize, channels;

    Result<> remakeEncoder();
    Result<> errcheck(const char* where);
};

#endif // GLOBED_VOICE_SUPPORT
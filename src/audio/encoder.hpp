#pragma once
#include <defs.hpp>

#if GLOBED_VOICE_SUPPORT

#include <opus.h>

#include <data/bytebuffer.hpp>

constexpr size_t VOICE_MAX_BYTES_IN_FRAME = 1000;

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

    AudioEncoder(AudioEncoder&& other) noexcept {
        encoder = other.encoder;
        other.encoder = nullptr;

        channels = other.channels;
        sampleRate = other.sampleRate;
        frameSize = other.frameSize;
    }

    AudioEncoder& operator=(AudioEncoder&& other) noexcept {
        if (this != &other) {
            if (this->encoder) {
                opus_encoder_destroy(this->encoder);
            }

            this->encoder = other.encoder;
            other.encoder = nullptr;

            channels = other.channels;
            sampleRate = other.sampleRate;
            frameSize = other.frameSize;
        }

        return *this;
    }

    // Encode the given PCM samples with Opus. The amount of samples passed must be equal to `frameSize` passed in the constructor.
    // After you no longer need the encoded data, you must call `data.freeData()`, or (preferrably, for explicitness) `AudioEncoder::freeData(data)`
    [[nodiscard]] EncodedOpusData encode(const float* data);

    // Free the underlying buffer of the encoded frame
    static void freeData(EncodedOpusData& data) {
        data.freeData();
    }

    // sets the sample rate that will be used and recreates the encoder
    void setSampleRate(int sampleRate);
    // sets the frame size of the data that will be used
    void setFrameSize(int frameSize);
    // sets the amount of channels that will be used and recreates the encoder
    void setChannels(int channels);

private:
    // EXPERIMENTAL ZONE
    //
    // those functions are here for completeness sake, right now they are NOT used anywhere.
    // in future they may be configurable by the end user, in which case verify that the user cannot screw things up
    // and remove the `private:` specifier and this comment.

    // resets the internal state of the encoder
    void resetState();

    // sets the bitrate for the encoder
    void setBitrate(int bitrate);

    // sets the encoder complexity (1-10)
    void setComplexity(int complexity);

    // sets whether to use VBR or CBR (if false)
    void setVariableBitrate(bool variablebr = true);

protected:
    OpusEncoder* encoder = nullptr;

    int _res;
    int sampleRate, frameSize, channels;

    void remakeEncoder();
    void errcheck(const char* where);
};

#endif // GLOBED_VOICE_SUPPORT
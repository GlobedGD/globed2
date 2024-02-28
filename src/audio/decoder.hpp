#pragma once
#include <defs/platform.hpp>

#if GLOBED_VOICE_SUPPORT

#include <defs/minimal_geode.hpp>
#include <util/data.hpp>

#include "encoder.hpp"

struct OpusDecoder;

struct DecodedOpusData {
    float* ptr;
    size_t length;

    void freeData();
};

class AudioDecoder {
public:
    AudioDecoder(int sampleRate = 0, int frameSize = 0, int channels = 1);
    ~AudioDecoder();

    // disable copying and enable moving
    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    AudioDecoder(AudioDecoder&& other) noexcept;
    AudioDecoder& operator=(AudioDecoder&& other) noexcept;

    // Decodes the given Opus data into PCM float samples. `length` must be the size of the input data in bytes.
    // After you no longer need the decoded data, you must call `data.freeData()`, or (preferrably, for explicitness) `AudioDecoder::freeData(data)`
    [[nodiscard]] Result<DecodedOpusData> decode(const util::data::byte* data, size_t length);

    // Decodes the given Opus data into PCM float samples. `length` must be the size of the input data in bytes.
    // After you no longer need the decoded data, you must call `data.freeData()`, or (preferrably, for explicitness) `AudioDecoder::freeData(data)`
    [[nodiscard]] Result<DecodedOpusData> decode(const EncodedOpusData& data);

    static void freeData(DecodedOpusData& data) {
        data.freeData();
    }

    // sets the sample rate that will be used and recreates the decoder
    Result<> setSampleRate(int sampleRate);
    // sets the frame size of the data that will be used
    void setFrameSize(int frameSize);
    // sets the amount of channels that will be used and recreates the decoder
    Result<> setChannels(int channels);

private:
    // EXPERIMENTAL ZONE
    // See the note in `encoder.hpp`

    // reset the internal decoder state
    Result<> resetState();

protected:
    OpusDecoder* decoder = nullptr;

    int _res;
    int sampleRate, frameSize, channels;

    Result<> remakeDecoder();
    Result<> errcheck(const char* where);
};

#endif // GLOBED_VOICE_SUPPORT
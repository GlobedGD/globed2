#pragma once

#include <globed/prelude.hpp>
#include <memory>

struct OpusDecoder;

namespace globed {

struct EncodedOpusData;

struct DecodedOpusData {
    std::shared_ptr<float[]> data;
    size_t size;
};

class GLOBED_DLL AudioDecoder {
public:
    AudioDecoder(int sampleRate = 0, int frameSize = 0, int channels = 1);
    ~AudioDecoder();

    // disable copying and enable moving
    AudioDecoder(const AudioDecoder &) = delete;
    AudioDecoder &operator=(const AudioDecoder &) = delete;

    AudioDecoder(AudioDecoder &&other) noexcept;
    AudioDecoder &operator=(AudioDecoder &&other) noexcept;

    // Decodes the given Opus data into PCM float samples. `length` must be the size of the input data in bytes.
    [[nodiscard]] Result<DecodedOpusData> decode(const uint8_t *data, size_t length);
    [[nodiscard]] Result<DecodedOpusData> decode(const EncodedOpusData &frame);

    // sets the sample rate that will be used and recreates the decoder
    Result<> setSampleRate(int sampleRate);
    // sets the frame size of the data that will be used
    void setFrameSize(int frameSize);
    // sets the amount of channels that will be used and recreates the decoder
    Result<> setChannels(int channels);

private:
    // EXPERIMENTAL ZONE
    // See the note in `AudioEncoder.hpp`

    // reset the internal decoder state
    Result<> resetState();

protected:
    OpusDecoder *m_decoder = nullptr;

    int m_sampleRate, m_frameSize, m_channels;

    Result<> remakeDecoder();
    static std::string_view errorToString(int code);
};

} // namespace globed
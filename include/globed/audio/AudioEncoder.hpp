#pragma once

#include <globed/prelude.hpp>
#include <memory>

struct OpusEncoder;

namespace globed {

struct EncodedOpusData {
    std::shared_ptr<uint8_t[]> data;
    size_t size;
};

class GLOBED_DLL AudioEncoder {
public:
    AudioEncoder(int sampleRate = 0, int frameSize = 0, int channels = 1);
    ~AudioEncoder();

    // disable copying and enable moving
    AudioEncoder(const AudioEncoder &) = delete;
    AudioEncoder &operator=(const AudioEncoder &) = delete;

    AudioEncoder(AudioEncoder &&other) noexcept;
    AudioEncoder &operator=(AudioEncoder &&other) noexcept;

    // Encode the given PCM samples with Opus. The amount of samples passed must be equal to `frameSize` passed in the
    // constructor.
    [[nodiscard]] Result<EncodedOpusData> encode(const float *data);

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
    OpusEncoder *m_encoder = nullptr;

    int m_sampleRate, m_frameSize, m_channels;

    Result<> remakeEncoder();
    static std::string_view errorToString(int code);

    template <typename... Args> Result<> encoderCtl(const char *name, Args... args)
    {
        auto res = opus_encoder_ctl(m_encoder, args...);
        if (res < 0) {
            return Err("opus_encoder_ctl({}) failed: {}", name, AudioEncoder::errorToString(res));
        }

        return Ok();
    }
};

} // namespace globed